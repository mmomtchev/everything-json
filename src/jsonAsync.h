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

struct JSONElementContext {
  // The input string
  shared_ptr<padded_string> input_text;

  // The containing document
  shared_ptr<parser> parser_;
  shared_ptr<element> document;

  // The root of this subvalue
  element root;
};

namespace ToObjectAsync {
// This is the state for the iterative tree traversal
// of ToObjectAsync
struct Element {
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
  Element(const element &_item) : item(_item), iterator({.object = {}}) {}
};

struct Context {
  Napi::Env env;
  Napi::Reference<Value> top;
  vector<Element *> &queue;
  function<void(Napi::Value)> resolve;
  function<void(exception_ptr)> reject;
};

};

class JSON : public ObjectWrap<JSON>, JSONElementContext {
  static Napi::Value ToObject(Napi::Env, const element &);
  static void ToObjectAsync(ToObjectAsync::Context *, high_resolution_clock::time_point);

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
