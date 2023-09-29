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

Object Init(Napi::Env env, Object exports) {
  Function JSON_ctor = JSON::GetClass(env);
  exports.Set("JSON", JSON_ctor);

  auto instance = new InstanceData;
  instance->JSON_ctor = Persistent(JSON_ctor);
  env.SetInstanceData(instance);

  return exports;
}

NODE_API_MODULE(JSONAsync, Init)
