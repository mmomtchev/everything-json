#include "jsonAsync.h"
#include <node_api.h>

Function JSON::GetClass(Napi::Env env) {
  return DefineClass(env, "JSON",
                     {
                         JSON::InstanceAccessor<&JSON::TypeGetter>("type"),
                         JSON::InstanceAccessor<&JSON::ToStringGetter>(Symbol::WellKnown(env, "toStringTag")),
                         JSON::InstanceMethod<&JSON::Get>("get"),
                         JSON::InstanceMethod<&JSON::Expand>("expand"),
                         JSON::InstanceMethod<&JSON::Path>("path"),
                         JSON::InstanceMethod<&JSON::ToObject>("toObject"),
                         JSON::InstanceMethod<&JSON::ToObjectAsync>("toObjectAsync"),
                         JSON::StaticMethod<&JSON::Parse>("parse"),
                         JSON::StaticMethod<&JSON::ParseAsync>("parseAsync"),
                         JSON::StaticAccessor<&JSON::LatencyGetter, &JSON::LatencySetter>("latency"),
                         JSON::StaticAccessor<&JSON::SIMDJSONVersionGetter>("simdjson_version"),
                         JSON::StaticAccessor<&JSON::SIMDGetter>("simd"),
#ifdef DEBUG
                         JSON::StaticValue("debug", Boolean::New(env, true)),
#endif
                     });
}

Object Init(Napi::Env env, Object exports) {
  Function JSON_ctor = JSON::GetClass(env);
  exports.Set("JSON", JSON_ctor);

  auto instance = new InstanceData;
  instance->JSON_ctor = Persistent(JSON_ctor);
  env.SetInstanceData(instance);

#ifdef DEBUG
  cerr << "everything-json environment initialization: " << instance << ":" << std::this_thread::get_id() << endl;
#endif

  uv_loop_t *loop;
  if (napi_get_uv_event_loop(env, &loop) != napi_ok) {
    throw Error::New(env, "Failed getting event loop");
  }
  uv_async_init(loop, &instance->runQueueJob, JSON::ProcessRunQueue);
  instance->runQueueJob.data = instance;
  uv_unref(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob));

  napi_status r = napi_add_async_cleanup_hook(
      env,
      [](napi_async_cleanup_hook_handle hook, void *arg) {
        auto instance = static_cast<InstanceData *>(arg);
#ifdef DEBUG
        cerr << "everything-json environment cleanup: " << instance << endl;
#endif
        instance->runQueueJob.data = hook;
        uv_close(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob), [](uv_handle_t *handle) {
          auto hook = static_cast<napi_async_cleanup_hook_handle>(handle->data);
          napi_status r = napi_remove_async_cleanup_hook(hook);
          if (r != napi_ok) {
            printf("Failed to unload the environment\n");
#ifdef DEBUG
            abort();
#endif
          }
        });
        instance->JSON_ctor.Reset();
      },
      instance, nullptr);
  if (r != napi_ok) {
    throw Error::New(env, "Failed registering a cleanup hook");
  }
  return exports;
}

NODE_API_MODULE(JSONAsync, Init)
