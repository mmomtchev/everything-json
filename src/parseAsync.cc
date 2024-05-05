#include "jsonAsync.h"

Value JSON::ParseAsync(const CallbackInfo &info) {
  class ParserAsyncWorker : public AsyncWorker {
    Promise::Deferred deferred;
    std::shared_ptr<padded_string> json_text;
    std::shared_ptr<parser> parser_;
    std::shared_ptr<element> document;

  public:
    ParserAsyncWorker(Napi::Env env, std::shared_ptr<padded_string> text)
        : AsyncWorker(env, "JSONAsyncWorker"), deferred(env), json_text(text) {}
    virtual void Execute() override {
      napi_env env = Env();
      parser_ = Napi::MakeTracking<parser>(env);
      // This needs https://github.com/simdjson/simdjson/issues/1017 for optimal solution
      document = Napi::MakeTracking<element>(env, json_text->length() * 2, parser_->parse(*json_text));
    }
    virtual void OnOK() override {
      Napi::Env env = Env();
      auto instance = env.GetInstanceData<InstanceData>();
      element root = *document.get();
      JSONElementContext context(env, json_text, parser_, document, root);
      napi_value ctor_args = External<JSONElementContext>::New(env, &context);
      auto result = New(instance, root, context.store_json.get(), &ctor_args);
      deferred.Resolve(result);
    }
    virtual void OnError(const Napi::Error &e) override { deferred.Reject(e.Value()); }
    Promise GetPromise() { return deferred.Promise(); }
  };

  Napi::Env env(info.Env());

  auto parser_ = Napi::MakeTracking<parser>(env);
  auto json_text = GetString(info);
  auto worker = new ParserAsyncWorker(env, json_text);

  worker->Queue();
  return worker->GetPromise();
}
