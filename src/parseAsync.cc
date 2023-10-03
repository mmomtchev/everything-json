#include "jsonAsync.h"

Value JSON::ParseAsync(const CallbackInfo &info) {
  class ParserAsyncWorker : public AsyncWorker {
    Promise::Deferred deferred;
    shared_ptr<padded_string> json_text;
    shared_ptr<parser> parser_;
    shared_ptr<element> document;

  public:
    ParserAsyncWorker(Napi::Env env, shared_ptr<padded_string> text)
        : AsyncWorker(env, "JSONAsyncWorker"), deferred(env), json_text(text) {}
    virtual void Execute() override {
      parser_ = make_shared<parser>();
      document = make_shared<element>(element(parser_->parse(*json_text)));
    }
    virtual void OnOK() override {
      Napi::Env env = Env();
      auto instance = env.GetInstanceData<InstanceData>();
      JSONElementContext context(json_text, parser_, document, *document.get());
      napi_value ctor_args = External<JSONElementContext>::New(env, &context);
      auto result = instance->JSON_ctor.Value().New(1, &ctor_args);
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
