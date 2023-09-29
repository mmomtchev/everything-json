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

  static Napi::Value ToObject(Napi::Env, const element &);

public:
  JSON(const CallbackInfo &);
  virtual ~JSON();

  static Napi::Value Parse(const CallbackInfo &);
  static Napi::Value ParseAsync(const CallbackInfo &);
  Napi::Value Get(const CallbackInfo &);
  Napi::Value ToObject(const CallbackInfo &);

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
  vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                  External<shared_ptr<element>>::New(env, &document),
                                  External<element>::New(env, document.get())};

  return instance->JSON_ctor.Value().New(ctor_args);
}

Value JSON::ParseAsync(const CallbackInfo &info) {
  class ParserAsyncWorker : public AsyncWorker {
    Promise::Deferred deferred;
    string json_text;
    shared_ptr<parser> parser_;
    shared_ptr<element> document;

  public:
    ParserAsyncWorker(Napi::Env env, const string &&text)
        : AsyncWorker(env, "JSONAsyncWorker"), deferred(env), json_text(move(text)) {}
    virtual void Execute() override {
      parser_ = make_shared<parser>();
      document = make_shared<element>(dom::element(parser_->parse(json_text)));
    }
    virtual void OnOK() override {
      Napi::Env env = Env();
      auto instance = env.GetInstanceData<InstanceData>();
      vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                      External<shared_ptr<element>>::New(env, &document),
                                      External<element>::New(env, document.get())};
      Napi::Value result = instance->JSON_ctor.Value().New(ctor_args);
      deferred.Resolve(result);
    }
    virtual void OnError(const Napi::Error &e) override { deferred.Reject(e.Value()); }
    Promise GetPromise() { return deferred.Promise(); }
  };

  Napi::Env env(info.Env());

  if (info.Length() != 1 || !info[0].IsString()) {
    auto deferred = Promise::Deferred::New(env);
    deferred.Reject(Error::New(env, "JSON.Parse expects a single string argument").Value());
    return deferred.Promise();
  }

  string json_text(info[0].ToString().Utf8Value());
  auto worker = new ParserAsyncWorker(env, move(json_text));

  worker->Queue();
  return worker->GetPromise();
}

Value JSON::Get(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  switch (root.type()) {
  case dom::element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);

    vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                    External<shared_ptr<element>>::New(env, &document), Value()};

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

    vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                    External<shared_ptr<element>>::New(env, &document), Value()};

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
  case dom::element_type::INT64:
  case dom::element_type::UINT64:
    return Number::New(env, (double)(root));
  case dom::element_type::BOOL:
    return Boolean::New(env, (bool)(root));
  case dom::element_type::NULL_VALUE:
    return env.Null();
  }

  throw Error::New(env, "Invalid JSON element");
}

Value JSON::ToObject(const CallbackInfo &info) { return ToObject(info.Env(), root); }

Value JSON::ToObject(Napi::Env env, const element &root) {
  EscapableHandleScope scope(env);
  Napi::Value result;

  switch (root.type()) {
  case dom::element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);
    size_t i = 0;
    for (dom::element child : dom::array(root)) {
      Napi::Value sub = ToObject(env, child);
      array.Set(i, sub);
      i++;
    }
    result = array;
    break;
  }
  case dom::element_type::OBJECT: {
    auto object = Object::New(env);
    for (auto field : dom::object(root)) {
      Napi::Value sub = ToObject(env, field.value);
      object.Set(field.key.data(), sub);
    }
    result = object;
    break;
  }
  case dom::element_type::STRING: {
    result = String::New(env, root.get_c_str());
    break;
  }
  case dom::element_type::DOUBLE:
  case dom::element_type::INT64:
  case dom::element_type::UINT64:
    result = Number::New(env, (double)(root));
    break;
  case dom::element_type::BOOL:
    result = Boolean::New(env, (bool)(root));
    break;
  case dom::element_type::NULL_VALUE:
    result = env.Null();
    break;
  default:
    throw Error::New(env, "Invalid JSON element");
  }
  return scope.Escape(result);
}

Function JSON::GetClass(Napi::Env env) {
  return DefineClass(env, "JSON",
                     {
                         JSON::InstanceMethod("get", &JSON::Get),
                         JSON::InstanceMethod("toObject", &JSON::ToObject),
                         JSON::StaticMethod("parse", &JSON::Parse),
                         JSON::StaticMethod("parseAsync", &JSON::ParseAsync),
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
