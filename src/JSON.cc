#include "jsonAsync.h"

JSON::JSON(const CallbackInfo &info) : ObjectWrap<JSON>(info) {
  Napi::Env env(info.Env());

  if (info.Length() != 3 || !info[0].IsExternal() || !info[1].IsExternal() || !info[2].IsExternal()) {
    throw Napi::Error::New(
        env, "JSON constructor cannot be called from JavaScript, use JSON.parse[Async] to parse a string");
  }

  parser_ = *info[0].As<External<shared_ptr<parser>>>().Data();
  document = *info[1].As<External<shared_ptr<element>>>().Data();
  root = *info[2].As<External<element>>().Data();
}

JSON::~JSON() {}

Value JSON::Parse(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  if (info.Length() != 1 || !info[0].IsString()) {
    throw Error::New(env, "JSON.Parse expects a single string argument");
  }

  auto parser_ = make_shared<parser>();
  auto document = make_shared<element>(element(parser_->parse(info[0].ToString().Utf8Value())));
  vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                  External<shared_ptr<element>>::New(env, &document),
                                  External<element>::New(env, document.get())};

  return instance->JSON_ctor.Value().New(ctor_args);
}

Value JSON::Get(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  switch (root.type()) {
  case element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);

    vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                    External<shared_ptr<element>>::New(env, &document), Value()};

    size_t i = 0;
    for (element child : dom::array(root)) {
      ctor_args[2] = External<element>::New(env, &child);
      Napi::Value sub = instance->JSON_ctor.Value().New(ctor_args);
      array.Set(i, sub);
      i++;
    }
    return array;
  }
  case element_type::OBJECT: {
    auto object = Object::New(env);

    vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                    External<shared_ptr<element>>::New(env, &document), Value()};

    for (auto field : dom::object(root)) {
      ctor_args[2] = External<element>::New(env, &field.value);
      Napi::Value sub = instance->JSON_ctor.Value().New(ctor_args);
      object.Set(field.key.data(), sub);
    }
    return object;
  }
  case element_type::STRING: {
    auto string = String::New(env, root.get_c_str());
    return string;
  }
  case element_type::DOUBLE:
  case element_type::INT64:
  case element_type::UINT64:
    return Number::New(env, (double)(root));
  case element_type::BOOL:
    return Boolean::New(env, (bool)(root));
  case element_type::NULL_VALUE:
    return env.Null();
  }

  throw Error::New(env, "Invalid JSON element");
}

Value JSON::ToObject(const CallbackInfo &info) { return ToObject(info.Env(), root); }

Value JSON::ToObject(Napi::Env env, const element &root) {
  EscapableHandleScope scope(env);
  Napi::Value result;

  switch (root.type()) {
  case element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);
    size_t i = 0;
    for (element child : dom::array(root)) {
      Napi::Value sub = ToObject(env, child);
      array.Set(i, sub);
      i++;
    }
    result = array;
    break;
  }
  case element_type::OBJECT: {
    auto object = Object::New(env);
    for (auto field : dom::object(root)) {
      Napi::Value sub = ToObject(env, field.value);
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
  return scope.Escape(result);
}
