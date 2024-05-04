#include "jsonAsync.h"

Value JSON::ParseAsync(const CallbackInfo &info) {
  class ParserAsyncWorker : public AsyncWorker {
    Promise::Deferred deferred;
    shared_ptr<padded_string> json_text;
    shared_ptr<parser> parser_;
    shared_ptr<element> document;
    uint64_t external_memory;

  public:
    ParserAsyncWorker(Napi::Env env, shared_ptr<padded_string> text)
        : AsyncWorker(env, "JSONAsyncWorker"), deferred(env), json_text(text), external_memory(0) {}
    virtual void Execute() override {
      parser_ = make_shared<parser>();
      napi_env env = Env();
      document = shared_ptr<element>(new element(parser_->parse(*json_text)),
                                     [env, external_memory = this->external_memory](void *p) {
                                       // Normally this should always get called on the main thread?
                                       Napi::MemoryManagement::AdjustExternalMemory(env, -external_memory);
                                       delete static_cast<element *>(p);
                                     });
    }
    virtual void OnOK() override {
      Napi::Env env = Env();
      auto instance = env.GetInstanceData<InstanceData>();
      element root = *document.get();
      JSONElementContext context(json_text, parser_, document, root);
      // This overreports memory to the GC
      external_memory = json_text->length() * 2;
      Napi::MemoryManagement::AdjustExternalMemory(env, external_memory);
      napi_value ctor_args = External<JSONElementContext>::New(env, &context);
      auto result = New(instance, root, context.store_json.get(), &ctor_args);
      deferred.Resolve(result);
    }
    virtual void OnError(const Napi::Error &e) override { deferred.Reject(e.Value()); }
    Promise GetPromise() { return deferred.Promise(); }
  };

  Napi::Env env(info.Env());

  auto parser_ = make_shared<parser>();
  auto json_text = GetString(info);
  auto worker = new ParserAsyncWorker(env, json_text);

  worker->Queue();
  return worker->GetPromise();
}
