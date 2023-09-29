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
  auto deferred = new Promise::Deferred(env);
  auto persistent = new Reference<Napi::Value>();
  *persistent = Persistent(info.This());

  ToObjectAsync(
      info.Env(), root,
      [persistent, deferred](Napi::Value element) {
        deferred->Resolve(element);
        persistent->Reset();
        delete persistent;
        delete deferred;
      },
      [persistent, deferred, env](exception_ptr err) {
        try {
          rethrow_exception(err);
        } catch (const exception &e) {
          deferred->Reject(Error::New(env, e.what()).Value());
        }
        persistent->Reset();
        delete persistent;
        delete deferred;
      },
      high_resolution_clock::now());

  return deferred->Promise();
}

void JSON::ToObjectAsync(Napi::Env env, const element &root, const function<void(Napi::Value)> resolve,
                         const function<void(exception_ptr)> reject, high_resolution_clock::time_point start) {
  HandleScope scope(env);
  Napi::Value result;

  try {
    switch (root.type()) {
    case element_type::ARRAY: {
      size_t len = dom::array(root).size();
      auto remaining = new size_t;
      (*remaining) = len;
      auto array = Array::New(env, len);
      auto arrayRef = new Reference<Array>();
      *arrayRef = Persistent(array);
      size_t i = 0;
      for (element child : dom::array(root)) {
        if (CanRun(start)) {
          ToObjectAsync(
              env, child,
              [remaining, resolve, i, arrayRef](Napi::Value sub) {
                (*remaining)--;
                arrayRef->Value().Set(i, sub);
                if (*remaining == 0) {
                  resolve(arrayRef->Value());
                  arrayRef->Reset();
                  delete arrayRef;
                  delete remaining;
                }
              },
              reject, start);
        } else {
          runQueue.push({env, child,
                         [remaining, resolve, i, arrayRef](Napi::Value sub) {
                           (*remaining)--;
                           arrayRef->Value().Set(i, sub);
                           if (*remaining == 0) {
                             resolve(arrayRef->Value());
                             arrayRef->Reset();
                             delete arrayRef;
                             delete remaining;
                           }
                         },
                         reject});
        }
        i++;
      }
      result = array;
      break;
    }
    case element_type::OBJECT: {
      auto object = Object::New(env);
      auto objectRef = new ObjectReference();
      *objectRef = Persistent(object);
      auto remaining = new size_t;
      (*remaining) = dom::object(root).size();
      for (auto field : dom::object(root)) {
        if (CanRun(start)) {
          ToObjectAsync(
              env, field.value,
              [remaining, resolve, key = field.key.data(), objectRef](Napi::Value sub) {
                (*remaining)--;
                objectRef->Value().Set(key, sub);
                if (*remaining == 0) {
                  resolve(objectRef->Value());
                  objectRef->Reset();
                  delete objectRef;
                  delete remaining;
                }
              },
              reject, start);
        } else {
          runQueue.push({env, field.value,
                         [remaining, resolve, key = field.key.data(), objectRef](Napi::Value sub) {
                           (*remaining)--;
                           objectRef->Value().Set(key, sub);
                           if (*remaining == 0) {
                             resolve(objectRef->Value());
                             objectRef->Reset();
                             delete objectRef;
                             delete remaining;
                           }
                         },
                         reject});
        }
      }
      result = object;
      break;
    }
    case element_type::STRING: {
      resolve(String::New(env, root.get_c_str()));
      break;
    }
    case element_type::DOUBLE:
    case element_type::INT64:
    case element_type::UINT64:
      resolve(Number::New(env, (double)(root)));
      break;
    case element_type::BOOL:
      resolve(Boolean::New(env, (bool)(root)));
      break;
    case element_type::NULL_VALUE:
      resolve(env.Null());
      break;
    default:
      throw Error::New(env, "Invalid JSON element");
    }
  } catch (...) {
    reject(current_exception());
  }

  if (!runQueue.empty()) {
    auto instance = env.GetInstanceData<InstanceData>();
    uv_async_send(&instance->runQueueJob);
    uv_ref(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob));
  }
}
