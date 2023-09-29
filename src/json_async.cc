#define NAPI_VERSION 6
#define SIMDJSON_EXCEPTIONS 1
#include "simdjson.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <napi.h>
#include <string>

using namespace Napi;
using namespace simdjson;
using namespace simdjson::dom;
using namespace std;
using namespace std::chrono;

struct InstanceData {
  FunctionReference JSON_ctor;
};

class JSON : public ObjectWrap<JSON> {
  // The containing document
  shared_ptr<parser> parser_;
  shared_ptr<element> document;

  // The root of this subvalue
  element root;

  Napi::Value Sub(element *sub);

public:
  JSON(const CallbackInfo &);
  virtual ~JSON();

  static Napi::Value Parse(const CallbackInfo &);
  Napi::Value Get(const CallbackInfo &);

  static Function GetClass(Napi::Env env);
};

/*
 */
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
  auto document = make_shared<element>(dom::element(parser_->parse(info[0].ToString().Utf8Value())));
  std::vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                        External<shared_ptr<element>>::New(env, &document),
                                        External<element>::New(env, document.get())};

  return instance->JSON_ctor.Value().New(ctor_args);
}

Value JSON::Get(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  switch (root.type()) {
  case dom::element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);

    std::vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                          External<shared_ptr<element>>::New(env, &document),
                                          Value()};

    size_t i = 0;
    for (dom::element child : dom::array(root)) {
      ctor_args[2] = External<element>::New(env, &child);
      Napi::Value sub = instance->JSON_ctor.Value().New(ctor_args);
      array.Set(i, sub);
      i++;
    }
    return array;
  }
  case dom::element_type::OBJECT: {
    auto object = Object::New(env);

    std::vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                          External<shared_ptr<element>>::New(env, &document),
                                          Value()};

    for (auto field : dom::object(root)) {
      ctor_args[2] = External<element>::New(env, &field.value);
      Napi::Value sub = instance->JSON_ctor.Value().New(ctor_args);
      object.Set(field.key.data(), sub);
    }
    return object;
  }
  case dom::element_type::STRING: {
    auto string = String::New(env, root.get_c_str());
    return string;
  }
  case dom::element_type::DOUBLE:
    return Number::New(env, (double)(root));
  case dom::element_type::INT64:
    return Number::New(env, (int64_t)(root));
  case dom::element_type::UINT64:
    return Number::New(env, (uint64_t)(root));
  case dom::element_type::BOOL:
    return Boolean::New(env, (bool)(root));
  case dom::element_type::NULL_VALUE:
    return env.Null();
  }

  throw Error::New(env, "Invalid JSON element");
}

Function JSON::GetClass(Napi::Env env) {
  return DefineClass(env, "JSON",
                     {
                         JSON::InstanceMethod("get", &JSON::Get),
                         JSON::StaticMethod("parse", &JSON::Parse),
                     });
}

Object Init(Napi::Env env, Object exports) {
  Function JSON_ctor = JSON::GetClass(env);
  exports.Set("JSON", JSON_ctor);

  auto instance = new InstanceData;
  instance->JSON_ctor = Persistent(JSON_ctor);
  env.SetInstanceData(instance);

  return exports;
}

NODE_API_MODULE(JSONAsync, Init)
