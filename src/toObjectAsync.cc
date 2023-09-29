#include "jsonAsync.h"

Value JSON::ToObjectAsync(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto deferred = new Promise::Deferred(env);

  ToObjectAsync(info.Env(), root, [deferred](Napi::Value element) {
    deferred->Resolve(element);
    delete deferred;
  },
  [deferred, env](exception_ptr err) {
    try {
      rethrow_exception(err);
    } catch (const exception &e) {
      deferred->Reject(Error::New(env, e.what()).Value());
    }
    delete deferred;
  });

  return deferred->Promise();
}

void JSON::ToObjectAsync(Napi::Env env, const element &root, std::function<void(Napi::Value)> resolve,
                         std::function<void(exception_ptr)> reject) {
  HandleScope scope(env);
  Napi::Value result;
  auto remaining = new size_t;
  *remaining = 0;

  try {
    switch (root.type()) {
    case element_type::ARRAY: {
      size_t len = dom::array(root).size();
      (*remaining) = len;
      auto array = Array::New(env, len);
      auto arrayRef = new Reference<Array>();
      *arrayRef = Persistent(array);
      size_t i = 0;
      for (element child : dom::array(root)) {
        //printf("Start array element (%lu)\n", *remaining);
        ToObjectAsync(
            env, child,
            [remaining, resolve, i, arrayRef](Napi::Value sub) {
              (*remaining)--;
              //printf("End array element (%lu)\n", *remaining);
              arrayRef->Value().Set(i, sub);
              if (*remaining == 0) {
                resolve(arrayRef->Value());
                arrayRef->Reset();
                delete arrayRef;
                delete remaining;
              }
            },
            reject);
        i++;
      }
      result = array;
      break;
    }
    case element_type::OBJECT: {
      auto object = Object::New(env);
      auto objectRef = new ObjectReference();
      *objectRef = Persistent(object);
      (*remaining) = dom::object(root).size();
      for (auto field : dom::object(root)) {
        //printf("Start object element (%lu)\n", *remaining);
        ToObjectAsync(
            env, field.value,
            [remaining, resolve, key = field.key.data(), objectRef](Napi::Value sub) {
              (*remaining)--;
              //printf("End object element (%lu)\n", *remaining);
              objectRef->Value().Set(key, sub);
              if (*remaining == 0) {
                resolve(objectRef->Value());
                objectRef->Reset();
                delete objectRef;
                delete remaining;
              }
            },
            reject);
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
}
