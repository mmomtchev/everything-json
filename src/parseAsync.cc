#include "jsonAsync.h"

Value JSON::ParseAsync(const CallbackInfo &info) {
  class ParserAsyncWorker : public AsyncWorker {
    Promise::Deferred deferred;
    string json_text;
    shared_ptr<parser> parser_;
    shared_ptr<element> document;

  public:
    ParserAsyncWorker(Napi::Env env, const string &&text)
        : AsyncWorker(env, "JSONAsyncWorker"), deferred(env), json_text(move(text)) {}
    virtual void Execute() override {
      parser_ = make_shared<parser>();
      document = make_shared<element>(element(parser_->parse(json_text)));
    }
    virtual void OnOK() override {
      Napi::Env env = Env();
      auto instance = env.GetInstanceData<InstanceData>();
      vector<napi_value> ctor_args = {External<shared_ptr<parser>>::New(env, &parser_),
                                      External<shared_ptr<element>>::New(env, &document),
                                      External<element>::New(env, document.get())};
      Napi::Value result = instance->JSON_ctor.Value().New(ctor_args);
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

  string json_text(info[0].ToString().Utf8Value());
  auto worker = new ParserAsyncWorker(env, move(json_text));

  worker->Queue();
  return worker->GetPromise();
}
