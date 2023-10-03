#include "jsonAsync.h"

JSONElementContext::JSONElementContext(const shared_ptr<padded_string> &_input_text, const shared_ptr<parser> &_parser_,
                                       const shared_ptr<element> &_document, const element &_root)
    : input_text(_input_text), parser_(_parser_), document(_document), root(_root) {}

JSONElementContext::JSONElementContext() {}

JSON::JSON(const CallbackInfo &info) : ObjectWrap<JSON>(info) {
  Napi::Env env(info.Env());

  if (info.Length() != 1 || !info[0].IsExternal()) {
    throw Napi::Error::New(
        env, "JSON constructor cannot be called from JavaScript, use JSON.parse[Async] to parse a string");
  }

  auto context = info[0].As<External<JSONElementContext>>().Data();
  input_text = context->input_text;
  parser_ = context->parser_;
  document = context->document;
  root = context->root;
}

JSON::~JSON() {}

shared_ptr<padded_string> JSON::GetString(const CallbackInfo &info) {
  Napi::Env env(info.Env());

  if (info.Length() != 1 || (!info[0].IsString() && !info[0].IsBuffer())) {
    throw Error::New(env, "JSON.Parse expects a single string or Buffer argument");
  }

  auto parser_ = make_shared<parser>();

  size_t json_len;
  if (info[0].IsString()) {
    napi_get_value_string_utf8(env, info[0], nullptr, 0, &json_len);
    auto json = make_shared<padded_string>(json_len);
    napi_get_value_string_utf8(env, info[0], json->data(), json_len + 1, nullptr);
    return json;
  } else if (info[0].IsBuffer()) {
    auto buffer = info[0].As<Buffer<char>>();
    auto json = make_shared<padded_string>(buffer.Data() + buffer.ByteOffset(), buffer.ByteLength());
    return json;
  }

  throw Error::New(env, "JSON.Parse expects a single string or Buffer argument");
}

Value JSON::Parse(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  auto parser_ = make_shared<parser>();
  auto json = GetString(info);
  auto document = make_shared<element>(parser_->parse(*json));

  JSONElementContext context(json, parser_, document, *document.get());

  napi_value ctor_args = External<JSONElementContext>::New(env, &context);
  auto r = instance->JSON_ctor.Value().New(1, &ctor_args);
  return r;
}

Value JSON::Get(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  switch (root.type()) {
  case element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);

    JSONElementContext context(*this);
    napi_value ctor_args = External<JSONElementContext>::New(env, &context);

    size_t i = 0;
    for (element child : dom::array(root)) {
      context.root = child;
      auto sub = instance->JSON_ctor.Value().New(1, &ctor_args);
      array.Set(i, sub);
      i++;
    }
    return array;
  }
  case element_type::OBJECT: {
    auto object = Object::New(env);

    JSONElementContext context(*this);
    napi_value ctor_args = External<JSONElementContext>::New(env, &context);

    for (auto field : dom::object(root)) {
      context.root = field.value;
      auto sub = instance->JSON_ctor.Value().New(1, &ctor_args);
      object.Set(field.key.data(), sub);
    }
    return object;
  }
  case element_type::STRING: {
    auto string = String::New(env, root.get_c_str());
    return string;
  }
  case element_type::DOUBLE:
  case element_type::INT64:
  case element_type::UINT64:
    return Number::New(env, (double)(root));
  case element_type::BOOL:
    return Boolean::New(env, (bool)(root));
  case element_type::NULL_VALUE:
    return env.Null();
  }

  throw Error::New(env, "Invalid JSON element");
}

Value JSON::ToObject(const CallbackInfo &info) { return ToObject(info.Env(), root); }

Value JSON::ToObject(Napi::Env env, const element &root) {
  EscapableHandleScope scope(env);
  Napi::Value result;

  switch (root.type()) {
  case element_type::ARRAY: {
    size_t len = dom::array(root).size();
    auto array = Array::New(env, len);
    size_t i = 0;
    for (element child : dom::array(root)) {
      Napi::Value sub = ToObject(env, child);
      array.Set(i, sub);
      i++;
    }
    result = array;
    break;
  }
  case element_type::OBJECT: {
    auto object = Object::New(env);
    for (auto field : dom::object(root)) {
      Napi::Value sub = ToObject(env, field.value);
      object.Set(field.key.data(), sub);
    }
    result = object;
    break;
  }
  case element_type::STRING: {
    result = String::New(env, root.get_c_str());
    break;
  }
  case element_type::DOUBLE:
  case element_type::INT64:
  case element_type::UINT64:
    result = Number::New(env, (double)(root));
    break;
  case element_type::BOOL:
    result = Boolean::New(env, (bool)(root));
    break;
  case element_type::NULL_VALUE:
    result = env.Null();
    break;
  default:
    throw Error::New(env, "Invalid JSON element");
  }
  return scope.Escape(result);
}
