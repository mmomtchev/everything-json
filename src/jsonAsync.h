#ifndef JSON_ASYNC_H
#define JSON_ASYNC_H
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
  static Napi::Value ToObjectAsync(Napi::Env, const element &, size_t &remaining);

public:
  JSON(const CallbackInfo &);
  virtual ~JSON();

  static Napi::Value Parse(const CallbackInfo &);
  static Napi::Value ParseAsync(const CallbackInfo &);
  Napi::Value Get(const CallbackInfo &);
  Napi::Value ToObject(const CallbackInfo &);
  Napi::Value ToObjectAsync(const CallbackInfo &);

  static Function GetClass(Napi::Env env);
};
#endif
