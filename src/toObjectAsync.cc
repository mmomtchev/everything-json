#include "jsonAsync.h"

namespace ToObjectAsync {

Element::Element(const element &_item) : item(_item), iterator({{}}) {}
Context::Context(Napi::Env _env, Napi::Value _self)
    : env(_env), self(Persistent(_self)), top(), stack(), deferred(env) {}

} // namespace ToObjectAsync

inline bool JSON::CanRun(const high_resolution_clock::time_point &start) {
#ifdef DEBUG_VERBOSE
  return true;
#else
  return duration_cast<milliseconds>(high_resolution_clock::now() - start).count() < latency;
#endif
}

// Process the micro task queue
// (this is a process.nextTick re-implemented in C++)
void JSON::ProcessRunQueue(uv_async_t *handle) {
  const auto start(high_resolution_clock::now());

  auto instance = static_cast<InstanceData *>(handle->data);
  while (!instance->runQueue.empty() && CanRun(start)) {
    // An operation that has finished will have its state
    // deleted by args going out of scope
    auto args = instance->runQueue.front();
    ToObjectAsync(args, start);
    instance->runQueue.pop();
  }

  if (!instance->runQueue.empty()) {
    // More work, ask libuv to call us back after
    // one full event loop iteration
    uv_async_send(handle);
  } else {
    // No more work, do not block the process exit
    uv_unref(reinterpret_cast<uv_handle_t *>(handle));
  }

  std::lock_guard{instance->lock};
  if (instance->pendingExternalMemoryAdjustment != 0) {
    Napi::MemoryManagement::AdjustExternalMemory(instance->env, instance->pendingExternalMemoryAdjustment);
    instance->pendingExternalMemoryAdjustment = 0;
  }
}

// Main JS entry point
Value JSON::ToObjectAsync(const CallbackInfo &info) {
  Napi::Env env(info.Env());

  // The ToObjectAsync state is created here and it exists
  // as long as it sits on the queue
  auto state = Napi::MakeTracking<ToObjectAsync::Context>(env, 0, env, info.This());
  state->stack.emplace_back(root);
  ToObjectAsync(state, high_resolution_clock::now());

  return state->deferred.Promise();
}

#define LAST(v) (&(v).end()[-1])
#define PENULT(v) (((v).size() > 1) ? (&(v).end()[-2]) : nullptr)

// The actual implementation, called from the JS entry point
// and the task queue loop, runs until it is allowed, keeps its
// context in std::shared_ptr<ToObjectAsync::Context> state
// (this is an iterative heterogenous tree traversal)
void JSON::ToObjectAsync(std::shared_ptr<ToObjectAsync::Context> state, high_resolution_clock::time_point start) {
  Napi::Env env = state->env;
  auto &stack = state->stack;

  HandleScope scope(env);
  Napi::Value result;

  ToObjectAsync::Element *current, *previous;
  try {
    current = LAST(stack);
    previous = PENULT(stack);
    // Loop invariant at the beginning:
    // * current->item holds the currently evaluated item
    // * previous->item / previous->iterator hold its slot in the parent object/array

    // Evaluate the item and create a JS representation
    do {
      switch (current->item.type()) {
      case element_type::ARRAY: {
        size_t len = dom::array(current->item).size();
        auto array = Array::New(env, len);
        current->ref = Persistent<Napi::Value>(array);
        result = array;
        break;
      }
      case element_type::OBJECT: {
        auto object = Object::New(env);
        current->ref = Persistent<Napi::Value>(object);
        result = object;
        break;
      }
      case element_type::STRING: {
        result = String::New(env, current->item.get_c_str());
        break;
      }
      case element_type::DOUBLE:
      case element_type::INT64:
      case element_type::UINT64:
        result = Number::New(env, (double)(current->item));
        break;
      case element_type::BOOL:
        result = Boolean::New(env, (bool)(current->item));
        break;
      case element_type::NULL_VALUE:
        result = env.Null();
        break;
      default:
        throw Error::New(env, "Invalid JSON element");
      }

      // Set the obtained JS value at the upper level in its
      // parent slot: object, array or the top
      if (!previous) {
        state->top = Persistent(result);
      } else {
        switch (previous->item.type()) {
        case element_type::ARRAY: {
          Array array = previous->ref.Value().As<Array>();
          array.Set(previous->idx, result);
#ifdef DEBUG_VERBOSE
          printf("%.*s [%u] = %s\n", (int)stack.size(), "                       ", (unsigned)previous->idx,
                 result.As<String>().Utf8Value().c_str());
#endif
          break;
        }
        case element_type::OBJECT: {
          Object object = previous->ref.Value().As<Object>();
          auto key = (*previous->iterator.object.idx).key.data();
          object.Set(key, result);
#ifdef DEBUG_VERBOSE
          printf("%.*s {%s} = %s\n", (int)stack.size(), "                       ", key,
                 result.As<String>().Utf8Value().c_str());
#endif
          break;
        }
        default:
          throw Error::New(env, "Internal error");
        }
      }

      // Go to the next element in the tree
      switch (current->item.type()) {
      // Array / Object -> recurse down
      case element_type::ARRAY:
        current->iterator.array.idx = dom::array(current->item).begin();
        current->iterator.array.end = dom::array(current->item).end();
        current->idx = 0;
        if (current->iterator.array.idx == current->iterator.array.end) {
          goto empty;
        }
        stack.emplace_back(*current->iterator.array.idx);
        current = LAST(stack);
        previous = PENULT(stack);
        break;
      case element_type::OBJECT:
        current->iterator.object.idx = dom::object(current->item).begin();
        current->iterator.object.end = dom::object(current->item).end();
        if (current->iterator.object.idx == current->iterator.object.end) {
          goto empty;
        }
        stack.emplace_back((*current->iterator.object.idx).value);
        current = LAST(stack);
        previous = PENULT(stack);
        break;

      default:
      empty:
        // Primitive values -> increment the parent iterator
        // and recurse up as many levels as needed
        bool backtracked = true;
        while (backtracked && previous) {
          backtracked = false;
          switch (previous->item.type()) {
          case element_type::ARRAY: {
            previous->idx++;
            previous->iterator.array.idx++;
            if (previous->iterator.array.idx == previous->iterator.array.end) {
              backtracked = true;
              stack.pop_back();
              current = LAST(stack);
              previous = PENULT(stack);
            } else {
              current->item = *previous->iterator.array.idx;
            }
            break;
          }
          case element_type::OBJECT: {
            previous->iterator.object.idx++;
            if (previous->iterator.object.idx == previous->iterator.object.end) {
              backtracked = true;
              stack.pop_back();
              current = LAST(stack);
              previous = PENULT(stack);
            } else {
              current->item = (*previous->iterator.object.idx).value;
            }
            break;
          }
          default:
            throw Error::New(env, "Internal error");
          }
        }
      }

      // if previous == nullptr here, we have successfully
      // recursed our way back to the top
    } while (previous && CanRun(start));
  } catch (const exception &err) {
    state->deferred.Reject(Error::New(env, err.what()).Value());
    return;
  }

  if (!previous) {
    assert(!state->top.IsEmpty());
    state->deferred.Resolve(state->top.Value());
  } else {
    // Put us back in the line
    auto instance = env.GetInstanceData<InstanceData>();
    instance->runQueue.push(state);
    // Do not allow the Node.js process to exit, we are not finished
    uv_ref(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob));
    // Ask libuv to call the micro task handler after one event loop iteration
    uv_async_send(&instance->runQueueJob);
  }
}
