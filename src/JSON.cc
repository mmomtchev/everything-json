#include "jsonAsync.h"
#include <sstream>

JSONElementContext::JSONElementContext(const shared_ptr<padded_string> &_input_text, const shared_ptr<parser> &_parser_,
                                       const shared_ptr<element> &_document, const element &_root)
    : input_text(_input_text), parser_(_parser_), document(_document), store_json(make_shared<ObjectStore>()),
      store_get(make_shared<ObjectStore>()), store_expand(make_shared<ObjectStore>()), root(_root) {}

JSONElementContext::JSONElementContext(const JSONElementContext &parent, const element &_root)
    : input_text(parent.input_text), parser_(parent.parser_), document(parent.document), store_json(parent.store_json),
      store_get(parent.store_get), store_expand(parent.store_expand), root(_root) {}

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
  store_json = context->store_json;
  store_get = context->store_get;
  store_expand = context->store_expand;
}

JSON::~JSON() {}

shared_ptr<padded_string> JSON::GetString(const CallbackInfo &info) {
  Napi::Env env(info.Env());

  if (info.Length() != 1 || (!info[0].IsString() && !info[0].IsBuffer())) {
    throw TypeError::New(env, "JSON.parse{Async} expects a single string or Buffer argument");
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

  throw TypeError::New(env, "JSON.Parse expects a single string or Buffer argument");
}

unsigned JSON::latency = 5;

Value JSON::LatencyGetter(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  return Number::New(env, latency);
}

void JSON::LatencySetter(const CallbackInfo &info, const Napi::Value &val) {
  Napi::Env env(info.Env());
  if (!val.IsNumber() || val.As<Number>().Int32Value() <= 0)
    throw TypeError::New(env, "Invalid value, must be a positive number in milliseconds");
  latency = val.As<Number>().Int32Value();
}

Value JSON::SIMDGetter(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  return String::New(env, get_active_implementation()->name());
}

Value JSON::SIMDJSONVersionGetter(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  return String::New(env, SIMDJSON_VERSION);
}

Value JSON::ToStringGetter(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  ostringstream type;
  type << "JSON<" << root.type() << ">";
  return String::New(env, type.str());
}

Value JSON::Parse(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  try {
    auto parser_ = make_shared<parser>();
    auto json = GetString(info);
    auto document = make_shared<element>(parser_->parse(*json));

    element root = *document.get();
    JSONElementContext context(json, parser_, document, root);
    napi_value ctor_args = External<JSONElementContext>::New(env, &context);
    return New(instance, root, context.store_json.get(), &ctor_args);
  } catch (const exception &err) {
    throw Error::New(env, err.what());
  }
}

Value JSON::GetPrimitive(Napi::Env env, const element &el) {
  switch (el.type()) {
  case element_type::STRING: {
    return String::New(env, el.get_c_str());
  }
  case element_type::DOUBLE:
  case element_type::INT64:
  case element_type::UINT64:
    return Number::New(env, (double)(el));
  case element_type::BOOL:
    return Boolean::New(env, (bool)(el));
  case element_type::NULL_VALUE:
    return env.Null();
  default:
    throw Error::New(env, "Invalid JSON element");
  }
  throw Error::New(env, "Invalid JSON element");
}

Value JSON::Get(Napi::Env env, bool expand) {
  ObjectStore *store = expand ? store_expand.get() : store_get.get();
  TRY_RETURN_FROM_STORE(store, root);

  auto instance = env.GetInstanceData<InstanceData>();
  Napi::Value sub;
  try {
    switch (root.type()) {
    case element_type::ARRAY: {
      size_t len = dom::array(root).size();
      auto array = Array::New(env, len);

      JSONElementContext context(*this);
      napi_value ctor_args = External<JSONElementContext>::New(env, &context);

      size_t i = 0;
      for (element child : dom::array(root)) {
        if (!expand || (child.is_array() || child.is_object())) {
          context.root = child;
          sub = New(instance, child, store_json.get(), &ctor_args);
        } else
          sub = GetPrimitive(env, child);
        array.Set(i, sub);
        i++;
      }
      store->emplace(root, move(Weak(array.As<Object>())));
      return array;
    }
    case element_type::OBJECT: {
      auto object = Object::New(env);

      JSONElementContext context(*this);
      napi_value ctor_args = External<JSONElementContext>::New(env, &context);

      for (auto field : dom::object(root)) {
        const auto &child = field.value;
        if (!expand || (child.is_array() || child.is_object())) {
          context.root = child;
          sub = New(instance, child, store_json.get(), &ctor_args);
        } else
          sub = GetPrimitive(env, child);
        object.Set(field.key.data(), sub);
      }
      store->emplace(root, move(Weak(object)));
      return object;
    }
    default:
      return GetPrimitive(env, root);
    }
  } catch (const exception &err) {
    throw Error::New(env, err.what());
  }

  throw Error::New(env, "Invalid JSON element");
}

Value JSON::Get(const CallbackInfo &info) { return Get(info.Env(), false); }
Value JSON::Expand(const CallbackInfo &info) { return Get(info.Env(), true); }
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

Value JSON::Path(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  auto instance = env.GetInstanceData<InstanceData>();

  if (info.Length() != 1 || !info[0].IsString()) {
    throw TypeError::New(env, "JSON.path expects a single string");
  }

  try {
    auto path = info[0].As<String>().Utf8Value();
    dom::element element = root.at_pointer(path);

    JSONElementContext context(*this, element);
    napi_value ctor_args = External<JSONElementContext>::New(env, &context);
    return New(instance, element, context.store_json.get(), &ctor_args);
  } catch (const exception &err) {
    throw Error::New(env, err.what());
  }
}

Value JSON::TypeGetter(const CallbackInfo &info) {
  Napi::Env env(info.Env());
  try {
    // This would have greatly benefited from String references in NAPI
    // Alas, I am currently blocked from discussions in Node.js as
    // part of an extortion/intimidation for an affair involving corruption
    // in the French police and judicial system in which the Node.js core
    // team is involved
    // (I was blocked for https://github.com/nodejs/node-gyp/issues/2903)
    switch (root.type()) {
    case element_type::ARRAY:
      return String::New(env, "array");
    case element_type::OBJECT:
      return String::New(env, "object");
    case element_type::STRING:
      return String::New(env, "string");
    case element_type::DOUBLE:
    case element_type::INT64:
    case element_type::UINT64:
      return String::New(env, "number");
    case element_type::BOOL:
      return String::New(env, "boolean");
    case element_type::NULL_VALUE:
      return String::New(env, "null");
    default:
      throw Error::New(env, "Invalid JSON element");
    }
  } catch (const exception &err) {
    throw Error::New(env, err.what());
  }
}
