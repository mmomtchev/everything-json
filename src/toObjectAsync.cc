#include "jsonAsync.h"

struct ToObjectAsyncArgs {
  Napi::Env env;
  const element root;
  const function<void(Napi::Value)> resolve;
  const function<void(exception_ptr)> reject;
};

queue<ToObjectAsyncArgs> runQueue;

static inline bool CanRun(const high_resolution_clock::time_point &start) {
  return duration_cast<milliseconds>(high_resolution_clock::now() - start).count() < 5;
}

void JSON::ProcessRunQueue(uv_async_t *handle) {
  const auto start(high_resolution_clock::now());

  while (!runQueue.empty() && CanRun(start)) {
    auto const &args = runQueue.front();
    ToObjectAsync(args.env, args.root, args.resolve, args.reject, start);
    runQueue.pop();
  }

  if (!runQueue.empty()) {
    uv_async_send(handle);
  } else {
    uv_unref(reinterpret_cast<uv_handle_t *>(handle));
  }
}

Value JSON::ToObjectAsync(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto deferred = make_shared<Promise::Deferred>(env);
  auto persistent = new Reference<Napi::Value>();
  *persistent = Persistent(info.This());

  ToObjectAsync(
      info.Env(), root,
      [persistent, deferred](Napi::Value element) {
        deferred->Resolve(element);
        persistent->Reset();
        delete persistent;
      },
      [persistent, deferred, env](exception_ptr err) {
        try {
          rethrow_exception(err);
        } catch (const exception &e) {
          deferred->Reject(Error::New(env, e.what()).Value());
        }
        persistent->Reset();
        delete persistent;
      },
      high_resolution_clock::now());

  return deferred->Promise();
}

struct ToObjectAsyncContext {
  element item;
  union {
    struct {
      dom::array::iterator idx;
      dom::array::iterator end;
    } array;
    struct {
      dom::object::iterator idx;
      dom::object::iterator end;
    } object;
  } iterator;
  Reference<Value> ref;
  size_t idx;
  ToObjectAsyncContext(const element &_item) : item(_item), iterator({.object = {}}) {}
};

void JSON::ToObjectAsync(Napi::Env env, const element &root, const function<void(Napi::Value)> resolve,
                         const function<void(exception_ptr)> reject, high_resolution_clock::time_point start) {
  printf("In C++\n");
  HandleScope scope(env);
  Napi::Value result;
  Napi::Value top;
  Object object;

  ToObjectAsyncContext *current = new ToObjectAsyncContext(root);
  vector<ToObjectAsyncContext *> queue{current};
  ToObjectAsyncContext *previous = nullptr;

  try {
    // Loop invariant at the beginning:
    // * current->item holds the currently evaluated item
    // * previous->item / previous->iterator hold its slot in the parent object/array
    do {
      switch (current->item.type()) {
        // Array / Object -> add to the queue (recurse down) and restart the loop
      case element_type::ARRAY: {
        Array array;
        size_t len = dom::array(current->item).size();
        array = Array::New(env, len);
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
        top = result;
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
        current = new ToObjectAsyncContext(*current->iterator.array.idx);
        queue.push_back(current);
        break;
      case element_type::OBJECT:
        current->iterator.object.idx = dom::object(current->item).begin();
        current->iterator.object.end = dom::object(current->item).end();
        previous = current;
        current = new ToObjectAsyncContext((*current->iterator.object.idx).value);
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
    } while (previous);
  } catch (...) {
    reject(current_exception());
  }

  delete(current);
  resolve(top);

  /*if (!runQueue.empty()) {
    auto instance = env.GetInstanceData<InstanceData>();
    uv_async_send(&instance->runQueueJob);
    uv_ref(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob));
  }*/
}
