#ifndef JSON_ASYNC_H
#define JSON_ASYNC_H
#define NAPI_VERSION 6
#define SIMDJSON_EXCEPTIONS 1
#include "simdjson.h"
#include <chrono>
#include <functional>
#include <napi.h>
#include <queue>
#include <string>
#include <uv.h>

using namespace Napi;
using namespace simdjson;
using namespace simdjson::dom;
using namespace std;
using namespace chrono;

struct InstanceData {
  FunctionReference JSON_ctor;
  uv_async_t runQueueJob;
};

class JSON : public ObjectWrap<JSON> {
  // The containing document
  shared_ptr<parser> parser_;
  shared_ptr<element> document;

  // The root of this subvalue
  element root;

  static Napi::Value ToObject(Napi::Env, const element &);
  static void ToObjectAsync(Napi::Env, const element &, const function<void(Napi::Value)>,
                            const function<void(exception_ptr)>);

public:
  JSON(const CallbackInfo &);
  virtual ~JSON();

  static Napi::Value Parse(const CallbackInfo &);
  static Napi::Value ParseAsync(const CallbackInfo &);
  Napi::Value Get(const CallbackInfo &);
  Napi::Value ToObject(const CallbackInfo &);
  Napi::Value ToObjectAsync(const CallbackInfo &);

  static void ProcessRunQueue(uv_async_t *);

  static Function GetClass(Napi::Env env);
};
#endif
