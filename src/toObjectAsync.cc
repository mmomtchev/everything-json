#include "jsonAsync.h"

Value JSON::ToObjectAsync(const CallbackInfo &info) {
  size_t remaining = 1;
  return ToObjectAsync(info.Env(), root, remaining);
}

Value JSON::ToObjectAsync(Napi::Env env, const element &root, size_t &remaining) {
  EscapableHandleScope scope(env);
  Napi::Value result;

  switch (root.type()) {
  case element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);
    size_t i = 0;
    for (element child : dom::array(root)) {
      remaining++;
      Napi::Value sub = ToObjectAsync(env, child, remaining);
      array.Set(i, sub);
      i++;
    }
    result = array;
    break;
  }
  case element_type::OBJECT: {
    auto object = Object::New(env);
    for (auto field : dom::object(root)) {
      remaining++;
      Napi::Value sub = ToObjectAsync(env, field.value, remaining);
      object.Set(field.key.data(), sub);
    }
    result = object;
    break;
  }
  case element_type::STRING: {
    result = String::New(env, root.get_c_str());
    break;
  }
  case element_type::DOUBLE:
  case element_type::INT64:
  case element_type::UINT64:
    result = Number::New(env, (double)(root));
    break;
  case element_type::BOOL:
    result = Boolean::New(env, (bool)(root));
    break;
  case element_type::NULL_VALUE:
    result = env.Null();
    break;
  default:
    throw Error::New(env, "Invalid JSON element");
  }

  remaining--;
  return scope.Escape(result);
}
