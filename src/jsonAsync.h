#ifndef JSON_ASYNC_H
#define JSON_ASYNC_H
#define SIMDJSON_EXCEPTIONS 1
#include "simdjson.h"

#include <chrono>
#include <functional>
#include <map>
#include <queue>
#include <string>
#include <mutex>

#define NAPI_VERSION 8
#include <napi.h>
#include <uv.h>

using namespace Napi;
using namespace simdjson;
using namespace simdjson::dom;
using namespace std;
using namespace chrono;

typedef map<element, ObjectReference> ObjectStore;

/**
 * The internal information required to identify a JSON element
 * in the simdjson parsed binary representation.
 *
 * All JSON elements in the same document share the same pointers
 * to the input text, the parser and the main root element (the document root).
 * They also share the same object stores.
 *
 * root is the root of the element.
 */
struct JSONElementContext {
  // The input string
  std::shared_ptr<padded_string> input_text;

  // The containing document
  std::shared_ptr<parser> parser_;
  std::shared_ptr<element> document;

  // The object store - contains weak refs to objects returned to JS
  std::shared_ptr<ObjectStore> store_json, store_get, store_expand;

  // The root of this subvalue
  element root;

  JSONElementContext(Napi::Env env, const std::shared_ptr<padded_string> &, const std::shared_ptr<parser> &,
                     const std::shared_ptr<element> &, const element &);
  JSONElementContext(const JSONElementContext &parent, const element &);
  JSONElementContext();
};

namespace ToObjectAsync {

/**
 * This is one element of the stack for the state for the iterative tree
 * traversal of ToObjectAsync
 */
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
  Element(const element &);
};

/**
 * This is the state information for the iterative tree
 * traversal of ToObjectAsync
 */
struct Context {
  Napi::Env env;
  // The JSON wrapped object
  Napi::Reference<Value> self;
  // The root of the constructed JS object
  Napi::Reference<Value> top;
  // The iterative traversal stack
  // (it is a vector because we need to access the last two elements)
  vector<Element> stack;
  Promise::Deferred deferred;
  Context(Napi::Env, Napi::Value);
};

}; // namespace ToObjectAsync

struct InstanceData {
  queue<std::shared_ptr<ToObjectAsync::Context>> runQueue;
  FunctionReference JSON_ctor;
  uv_async_t runQueueJob;
  std::mutex lock;
  int64_t pendingExternalMemoryAdjustment;
};

namespace Napi {

/**
 * An std::make_shared derivative that can track memory
 * through the V8 GC
 */

template <typename T, typename... ARGS>
inline std::shared_ptr<T> MakeTracking(Env env, int64_t extra_size, ARGS &&...args) {
  auto instance = env.GetInstanceData<InstanceData>();
  std::lock_guard{instance->lock};
  instance->pendingExternalMemoryAdjustment += extra_size + sizeof(T);
  return std::shared_ptr<T>{new T(std::forward<ARGS>(args)...), [instance, extra_size](void *p) {
                              std::lock_guard{instance->lock};
                              instance->pendingExternalMemoryAdjustment -= extra_size + sizeof(T);
                              delete static_cast<T *>(p);
                            }};
}
template <typename T> inline std::shared_ptr<T> MakeTracking(Env env) {
  auto instance = env.GetInstanceData<InstanceData>();
  std::lock_guard{instance->lock};
  instance->pendingExternalMemoryAdjustment += sizeof(T);
  return std::shared_ptr<T>{new T, [instance](void *p) {
                              std::lock_guard{instance->lock};
                              instance->pendingExternalMemoryAdjustment -= sizeof(T);
                              delete static_cast<T *>(p);
                            }};
}
} // namespace Napi

/**
 * The JavaScript proxy object for a JSON element in the binary parsed
 * structure of simdjson.
 */
class JSON : public ObjectWrap<JSON>, JSONElementContext {
  static unsigned latency;

  static inline Napi::Value New(InstanceData *, const element &, ObjectStore *store, const napi_value *);

  static Napi::Value ToObject(Napi::Env, const element &);
  static void ToObjectAsync(std::shared_ptr<ToObjectAsync::Context>, high_resolution_clock::time_point);
  static std::shared_ptr<padded_string> GetString(const CallbackInfo &);
  static inline bool CanRun(const high_resolution_clock::time_point &);
  static inline Napi::Value GetPrimitive(Napi::Env, const element &);
  Napi::Value Get(Napi::Env, bool);

public:
  JSON(const CallbackInfo &);
  virtual ~JSON();

  static Napi::Value Parse(const CallbackInfo &);
  static Napi::Value ParseAsync(const CallbackInfo &);
  Napi::Value Get(const CallbackInfo &);
  Napi::Value Expand(const CallbackInfo &);
  Napi::Value Path(const CallbackInfo &);
  Napi::Value ToObject(const CallbackInfo &);
  Napi::Value ToObjectAsync(const CallbackInfo &);
  Napi::Value ToStringGetter(const CallbackInfo &);
  Napi::Value TypeGetter(const CallbackInfo &);
  static Napi::Value LatencyGetter(const CallbackInfo &);
  static void LatencySetter(const CallbackInfo &, const Napi::Value &);
  static Napi::Value SIMDGetter(const CallbackInfo &);
  static Napi::Value SIMDJSONVersionGetter(const CallbackInfo &);

  static void ProcessRunQueue(uv_async_t *);
  static void ProcessExternalMemory(Napi::Env env);

  static Function GetClass(Napi::Env env);
};

inline bool JSON::CanRun(const high_resolution_clock::time_point &start) {
#ifdef DEBUG_VERBOSE
  return true;
#else
  return duration_cast<milliseconds>(high_resolution_clock::now() - start).count() < latency;
#endif
}

#define TRY_RETURN_FROM_STORE(store, el)                                                                               \
  if ((store)->count(el)) {                                                                                            \
    assert((store)->count(el) == 1);                                                                                   \
    auto &ref = (store)->find(el)->second;                                                                             \
    if (!ref.IsEmpty() && !ref.Value().IsEmpty()) {                                                                    \
      return ref.Value();                                                                                              \
    } else {                                                                                                           \
      (store)->erase(el);                                                                                              \
    }                                                                                                                  \
  }

Napi::Value JSON::New(InstanceData *instance, const element &el, ObjectStore *store, const napi_value *context) {
  TRY_RETURN_FROM_STORE(store, el);
  Napi::Value r;
  r = instance->JSON_ctor.Value().New(1, context);
  store->emplace(el, Weak(r.As<Object>()));
  return r;
}
#endif
