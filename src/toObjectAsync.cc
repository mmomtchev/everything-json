#include "jsonAsync.h"

queue<ToObjectAsync::Context *> runQueue;

static inline bool CanRun(const high_resolution_clock::time_point &start) {
  return duration_cast<milliseconds>(high_resolution_clock::now() - start).count() < 5;
}

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

static void dummyResolve(Napi::Value) { abort(); }
static void dummyReject(exception_ptr) { abort(); }

Value JSON::ToObjectAsync(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto deferred = make_shared<Promise::Deferred>(env);
  auto persistent = new Reference<Napi::Value>();
  *persistent = Persistent(info.This());

  auto rootElement = new ToObjectAsync::Element(root);
  auto queue = new vector<ToObjectAsync::Element *>({rootElement});
  auto *state = new ToObjectAsync::Context({.env = env,
                                            .top = Napi::Reference<Napi::Value>(),
                                            .queue = *queue,
                                            // suppress a compiler warning
                                            .resolve = dummyResolve,
                                            .reject = dummyReject});

  state->resolve = [persistent, deferred, rootElement, queue, state](Napi::Value element) -> void {
    deferred->Resolve(element);
    persistent->Reset();
    delete persistent;
    delete rootElement;
    delete queue;
    delete state;
  };
  state->reject = [persistent, deferred, env, rootElement, queue, state](exception_ptr err) -> void {
    try {
      rethrow_exception(err);
    } catch (const exception &e) {
      deferred->Reject(Error::New(env, e.what()).Value());
    }
    persistent->Reset();
    delete persistent;
    delete rootElement;
    delete queue;
    delete state;
  };

  ToObjectAsync(state, high_resolution_clock::now());

  return deferred->Promise();
}

void JSON::ToObjectAsync(ToObjectAsync::Context *state, high_resolution_clock::time_point start) {
  Napi::Env env = state->env;
  auto &queue = state->queue;

  HandleScope scope(env);
  Napi::Value result;

  ToObjectAsync::Element *current, *previous;

  try {
    // Loop invariant at the beginning:
    // * current->item holds the currently evaluated item
    // * previous->item / previous->iterator hold its slot in the parent object/array
    current = queue.end()[-1];
    if (queue.size() > 1)
      previous = queue.end()[-2];
    else
      previous = nullptr;

    do {
      switch (current->item.type()) {
        // Array / Object -> add to the queue (recurse down) and restart the loop
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
      // Primitive values -> construct the value
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

      // Set the obtained value at the upper level in its
      // parent slot: object, array or the top (resolve)
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

      // Next element in the tree
      switch (current->item.type()) {
      // Array / Object -> recurse down
      case element_type::ARRAY:
        current->iterator.array.idx = dom::array(current->item).begin();
        current->iterator.array.end = dom::array(current->item).end();
        current->idx = 0;
        previous = current;
        current = new ToObjectAsync::Element(*current->iterator.array.idx);
        queue.push_back(current);
        break;
      case element_type::OBJECT:
        current->iterator.object.idx = dom::object(current->item).begin();
        current->iterator.object.end = dom::object(current->item).end();
        previous = current;
        current = new ToObjectAsync::Element((*current->iterator.object.idx).value);
        queue.push_back(current);
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
            } else {
              current->item = *previous->iterator.array.idx;
            }
            break;
          }
          case element_type::OBJECT: {
            previous->iterator.object.idx++;
            if (previous->iterator.object.idx == previous->iterator.object.end) {
              backtracked = true;
            } else {
              current->item = (*previous->iterator.object.idx).value;
            }
            break;
          }
          default:
            throw Error::New(env, "Internal error");
          }
          if (backtracked) {
            if (!current->ref.IsEmpty())
              current->ref.Reset();
            delete current;
            queue.pop_back();
            current = queue.end()[-1];
            if (queue.size() > 1)
              previous = queue.end()[-2];
            else
              previous = nullptr;
          }
        } while (backtracked && previous);
      }

      // if previous == nullptr here, we have successfully
      // recursed our way back to the top
    } while (previous && CanRun(start));
  } catch (...) {
    state->reject(current_exception());
  }

  if (!previous) {
    assert(!state->top.IsEmpty());
    state->resolve(state->top.Value());
  } else {
    runQueue.push(state);
    auto instance = env.GetInstanceData<InstanceData>();
    uv_ref(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob));
    uv_async_send(&instance->runQueueJob);
  }
}
