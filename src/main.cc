#include "jsonAsync.h"

Function JSON::GetClass(Napi::Env env) {
  return DefineClass(env, "JSON",
                     {
                         JSON::InstanceMethod<&JSON::Get>("get"),
                         JSON::InstanceMethod<&JSON::ToObject>("toObject"),
                         JSON::InstanceMethod<&JSON::ToObjectAsync>("toObjectAsync"),
                         JSON::StaticMethod<&JSON::Parse>("parse"),
                         JSON::StaticMethod<&JSON::ParseAsync>("parseAsync"),
                         JSON::StaticAccessor<&JSON::LatencyGetter, &JSON::LatencySetter>("latency"),
                     });
}

void Cleanup(InstanceData *instance) {
#ifdef DEBUG
  cerr << "everything-json environment cleanup: " << instance << endl;
#endif
  uv_close(reinterpret_cast<uv_handle_t *>(&instance->runQueueJob), nullptr);
  instance->JSON_ctor.Reset();
}

Object Init(Napi::Env env, Object exports) {
  Function JSON_ctor = JSON::GetClass(env);
  exports.Set("JSON", JSON_ctor);

  auto instance = new InstanceData;
  instance->JSON_ctor = Persistent(JSON_ctor);
  env.SetInstanceData(instance);

#ifdef DEBUG
  cerr << "everything-json environment initialization: " << instance << endl;
#endif

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
