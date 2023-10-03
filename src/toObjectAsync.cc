#include "jsonAsync.h"

queue<shared_ptr<ToObjectAsync::Context>> runQueue;
namespace ToObjectAsync {

Element::Element(const element &_item) : item(_item), iterator({{}}) {}
Context::Context(Napi::Env _env, Napi::Value _self)
    : env(_env), self(Persistent(_self)), top(), queue(), deferred(env) {}

} // namespace ToObjectAsync

inline bool JSON::CanRun(const high_resolution_clock::time_point &start) {
  return duration_cast<milliseconds>(high_resolution_clock::now() - start).count() < latency;
}

// Process the micro task queue
void JSON::ProcessRunQueue(uv_async_t *handle) {
  const auto start(high_resolution_clock::now());

  while (!runQueue.empty() && CanRun(start)) {
    auto args = runQueue.front();
    ToObjectAsync(args, start);
    runQueue.pop();
  }

  if (!runQueue.empty()) {
    uv_async_send(handle);
  } else {
    uv_unref(reinterpret_cast<uv_handle_t *>(handle));
  }
}

// Main JS entry point
Value JSON::ToObjectAsync(const CallbackInfo &info) {
  Napi::Env env(info.Env());

  auto state = make_shared<ToObjectAsync::Context>(env, info.This());
  state->queue.emplace_back(ToObjectAsync::Element(root));
  ToObjectAsync(state, high_resolution_clock::now());

  return state->deferred.Promise();
}

#define LAST(v)   (&(v).end()[-1])
#define PENULT(v) (((v).size() > 1) ? (&(v).end()[-2]) : nullptr)

// The actual implementation, called from the JS entry point
// and the task queue loop, runs until it is allowed, keeps its
// context in shared_ptr<ToObjectAsync::Context> state
// (this is an iterative heterogenous tree traversal)
void JSON::ToObjectAsync(shared_ptr<ToObjectAsync::Context> state, high_resolution_clock::time_point start) {
  Napi::Env env = state->env;
  auto &queue = state->queue;

  HandleScope scope(env);
  Napi::Value result;

  ToObjectAsync::Element *current, *previous;
  try {
    current = LAST(queue);
    previous = PENULT(queue);
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
          break;
        }
        case element_type::OBJECT: {
          Object object = previous->ref.Value().ToObject();
          auto key = (*previous->iterator.object.idx).key.data();
          object.Set(key, result);
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
        queue.emplace_back(ToObjectAsync::Element(*current->iterator.array.idx));
        current = LAST(queue);
        previous = PENULT(queue);
        break;
      case element_type::OBJECT:
        current->iterator.object.idx = dom::object(current->item).begin();
        current->iterator.object.end = dom::object(current->item).end();
        queue.emplace_back(ToObjectAsync::Element((*current->iterator.object.idx).value));
        current = LAST(queue);
        previous = PENULT(queue);
        break;

      default:
        // Primitive values -> increment the parent iterator
        // and recurse up as many levels as needed
        bool backtracked;
        do {
          backtracked = false;
          switch (previous->item.type()) {
          case element_type::ARRAY: {
            previous->idx++;
            previous->iterator.array.idx++;
            if (previous->iterator.array.idx == previous->iterator.array.end) {
              backtracked = true;
              if (!current->ref.IsEmpty())
                current->ref.Reset();
              queue.pop_back();
              current = LAST(queue);
              previous = PENULT(queue);
            } else {
              current->item = *previous->iterator.array.idx;
            }
            break;
          }
          case element_type::OBJECT: {
            previous->iterator.object.idx++;
            if (previous->iterator.object.idx == previous->iterator.object.end) {
              backtracked = true;
              if (!current->ref.IsEmpty())
                current->ref.Reset();
              queue.pop_back();
              current = LAST(queue);
              previous = PENULT(queue);
            } else {
              current->item = (*previous->iterator.object.idx).value;
            }
            break;
          }
          default:
            throw Error::New(env, "Internal error");
          }
        } while (backtracked && previous);
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
    runQueue.push(state);
    auto instance = env.GetInstanceData<InstanceData>();
    uv_ref(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob));
    uv_async_send(&instance->runQueueJob);
  }
}
