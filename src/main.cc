#include "jsonAsync.h"

Function JSON::GetClass(Napi::Env env) {
  return DefineClass(env, "JSON",
                     {
                         JSON::InstanceMethod("get", &JSON::Get),
                         JSON::InstanceMethod("toObject", &JSON::ToObject),
                         JSON::InstanceMethod("toObjectAsync", &JSON::ToObjectAsync),
                         JSON::StaticMethod("parse", &JSON::Parse),
                         JSON::StaticMethod("parseAsync", &JSON::ParseAsync),
                     });
}

void Cleanup(InstanceData *instance) {
  uv_close(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob), nullptr);
  instance->JSON_ctor.Reset();
}

Object Init(Napi::Env env, Object exports) {
  Function JSON_ctor = JSON::GetClass(env);
  exports.Set("JSON", JSON_ctor);

  auto instance = new InstanceData;
  instance->JSON_ctor = Persistent(JSON_ctor);
  env.SetInstanceData(instance);

  uv_loop_t *loop;
  if (napi_get_uv_event_loop(env, &loop) != napi_ok) {
    throw Error::New(env, "Failed getting event loop");
  }
  uv_async_init(loop, &instance->runQueueJob, JSON::ProcessRunQueue);
  instance->runQueueJob.data = instance;
  uv_unref(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob));

  env.AddCleanupHook(Cleanup, instance);
  return exports;
}

NODE_API_MODULE(JSONAsync, Init)
