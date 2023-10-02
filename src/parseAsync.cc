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

  if (info.Length() != 1 || !info[0].IsString()) {
    auto deferred = Promise::Deferred::New(env);
    deferred.Reject(Error::New(env, "JSON.Parse expects a single string argument").Value());
    return deferred.Promise();
  }

  auto parser_ = make_shared<parser>();
  size_t json_len;
  napi_get_value_string_utf8(env, info[0], nullptr, 0, &json_len);
  auto json_text = make_shared<padded_string>(json_len);
  napi_get_value_string_utf8(env, info[0], json_text->data(), json_len + 1, nullptr);
  auto worker = new ParserAsyncWorker(env, json_text);

  worker->Queue();
  return worker->GetPromise();
}
