#define SIMDJSON_EXCEPTIONS 1
#include "simdjson.h"
#include <chrono>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <map>
#include <node_api.h>
#include <string>

// using namespace Napi;
using namespace simdjson;
using namespace std::chrono;

std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

class Element;

typedef std::vector<Element *> CTypeArray;
typedef std::vector<std::pair<std::string, Element *> *> CTypeObject;
typedef std::u16string CTypeString;

// A parsed element node
// (transport vehicle between the background and the foreground)
class Element {
  union {
    void *ptr;
    double d;
    bool b;
  } data;

public:
  enum Type { Null, Object, Array, Bool, Number, String } type;

  Element(Type _type, void *_data) : data({_data}), type(_type) {}
  Element(bool _b) : data({.b = _b}), type(Bool) {}
  Element(double _d) : data({.d = _d}), type(Number) {}
  Element(std::nullptr_t) : data({nullptr}), type(Null) {}
  operator CTypeArray *() { return reinterpret_cast<CTypeArray *>(data.ptr); }
  operator CTypeObject *() { return reinterpret_cast<CTypeObject *>(data.ptr); }
  operator CTypeString *() { return reinterpret_cast<CTypeString *>(data.ptr); }
  operator bool() { return data.b; }
  operator double() { return data.d; }
};

// Cleanup
// Launched in a background thread
void deleteElementTree(Element *element) {
  switch (element->type) {
  case Element::Array: {
    auto v = (CTypeArray *)(*element);
    for (auto el : *v) {
      deleteElementTree(el);
    }
    delete v;
    delete element;
    return;
  }
  case Element::Object: {
    auto m = (CTypeObject *)(*element);
    for (auto field : *m) {
      deleteElementTree(field->second);
    }
    delete m;
    delete element;
    return;
  }
  case Element::String: {
    auto s = (CTypeString *)(*element);
    delete s;
    delete element;
    return;
  }
  case Element::Bool:
  case Element::Number:
  case Element::Null: {
    delete element;
    return;
  }
  }
}

// Create the V8 object from the parsed tree
// Launched on the main thread
napi_value makeObject(napi_env env, Element *element) {
  switch (element->type) {
  case Element::Array: {
    auto v = (CTypeArray *)(*element);
    napi_value arr;
    napi_create_array_with_length(env, v->size(), &arr);
    size_t i = 0;
    for (auto el : *v) {
      napi_set_element(env, arr, i, makeObject(env, el));
      i++;
    }
    return arr;
  }
  case Element::Object: {
    auto m = (CTypeObject *)(*element);
    napi_value obj;
    napi_create_object(env, &obj);
    for (auto field : *m) {
      napi_set_named_property(env, obj, field->first.c_str(), makeObject(env, field->second));
    }
    return obj;
  }
  case Element::String: {
    auto s = (CTypeString *)(*element);
    napi_value r;
    napi_create_string_utf16(env, s->data(), s->length(), &r);
    return r;
  }
  case Element::Number: {
    auto n = (double)(*element);
    napi_value r;
    napi_create_double(env, n, &r);
    return r;
  }
  case Element::Bool: {
    auto b = (bool)(*element);
    napi_value r;
    napi_get_boolean(env, b, &r);
    return r;
  }
  case Element::Null:
    napi_value r;
    napi_get_null(env, &r);
    return r;
  }
  napi_throw_error(env, nullptr, "Internal error: Unexpected JSON type");
  return napi_value();
}

// Create a binary parsed tree with UTF-16 strings
// Launched in a background thread
Element *makeElement(dom::element element) {
  Element *result = nullptr;

  switch (element.type()) {
  case dom::element_type::ARRAY: {
    size_t len = dom::array(element).size();
    auto array = new CTypeArray();
    array->reserve(len);
    for (dom::element child : dom::array(element)) {
      array->push_back(makeElement(child));
    }
    result = new Element(Element::Array, array);
    break;
  }
  case dom::element_type::OBJECT: {
    size_t len = dom::object(element).size();
    auto map = new CTypeObject();
    map->reserve(len);
    for (auto field : dom::object(element)) {
      auto item =
          new std::pair<std::string, Element *>({std::move(field.key.data()), makeElement(field.value)});
      map->push_back(item);
    }
    result = new Element(Element::Object, map);
    break;
  }
  case dom::element_type::STRING: {
    std::string_view s8 = element;
    auto s16 = new CTypeString(convert.from_bytes(std::string(s8)));
    result = new Element(Element::String, s16);
    break;
  }
  case dom::element_type::DOUBLE:
  case dom::element_type::INT64:
  case dom::element_type::UINT64: {
    auto n = new double(element);
    result = new Element(n);
    break;
  }
  case dom::element_type::BOOL: {
    auto n = new bool(element);
    result = new Element(n);
    break;
  }
  case dom::element_type::NULL_VALUE:
    result = new Element(nullptr);
    break;
  }

  return result;
}

// The async context
typedef struct {
  std::u16string *json_text;
  Element *document;
  napi_async_work work;
  napi_deferred deferred;
} ParseContext;


// Do the background thread processing, part 1
void parseRun(napi_env env, void *data) {
  auto context = reinterpret_cast<ParseContext *>(data);

  auto t1 = high_resolution_clock::now();
  std::string json_text_u8 = convert.to_bytes(*context->json_text);
  json_text_u8.reserve(json_text_u8.size() + SIMDJSON_PADDING + 1);
  auto t2 = high_resolution_clock::now();
  std::cout << "CONVERT " << duration_cast<milliseconds>(t2 - t1).count() << std::endl;

  t1 = high_resolution_clock::now();
  dom::parser parser;
  dom::element document = parser.parse(json_text_u8);
  t2 = high_resolution_clock::now();
  std::cout << "PARSE " << duration_cast<milliseconds>(t2 - t1).count() << std::endl;

  t1 = high_resolution_clock::now();
  Element *document_raw = makeElement(document);
  t2 = high_resolution_clock::now();
  std::cout << "MAKEELEMENT " << duration_cast<milliseconds>(t2 - t1).count() << std::endl;
  
  context->document = document_raw;
}

// Do the background thread processing, part 2
void parseCleanup(napi_env env, void *data) {
  auto context = reinterpret_cast<ParseContext *>(data);

  auto t1 = high_resolution_clock::now();
  deleteElementTree(context->document);
  auto t2 = high_resolution_clock::now();
  std::cout << "DELETE " << duration_cast<milliseconds>(t2 - t1).count() << std::endl;

  delete context;
}

// Do the back-on-the-main-thread processing and launch background part 2
void parseComplete(napi_env env, napi_status status, void *data) {
  auto context = reinterpret_cast<ParseContext *>(data);

  auto t1 = high_resolution_clock::now();
  napi_value document_v8 = makeObject(env, context->document);
  auto t2 = high_resolution_clock::now();
  std::cout << "MAKEOBJECT " << duration_cast<milliseconds>(t2 - t1).count() << std::endl;

  napi_resolve_deferred(env, context->deferred, document_v8);
  napi_delete_async_work(env, context->work);

  napi_value resource_name;
  napi_create_string_utf8(env, "json-async-worker", strlen("json-async-worker"), &resource_name);
  napi_create_async_work(env, nullptr, resource_name, parseCleanup, nullptr, context, &context->work);
  napi_queue_async_work(env, context->work);
}

// Main entry point from JS
napi_value Parse(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv;
  napi_get_cb_info(env, info, &argc, &argv, nullptr, nullptr);
  if (argc < 1) {
    napi_throw_error(env, nullptr, "No JSON string passed");
    return napi_value();
  }
  napi_valuetype argtype;
  napi_typeof(env, argv, &argtype);
  if (argtype != napi_string) {
    napi_throw_error(env, nullptr, "Argument is not a string");
    return napi_value();
  }

  auto t1 = high_resolution_clock::now();
  size_t json_text_u16_len;
  napi_get_value_string_utf16(env, argv, nullptr, 0, &json_text_u16_len);
  auto *json_text_u16 = new std::u16string();
  json_text_u16->reserve(json_text_u16_len + 1);
  json_text_u16->resize(json_text_u16_len);
  napi_get_value_string_utf16(env, argv, &(*json_text_u16)[0], json_text_u16->capacity(), nullptr);
  json_text_u16->reserve(json_text_u16_len + SIMDJSON_PADDING + 1);
  auto t2 = high_resolution_clock::now();
  std::cout << "U16 " << duration_cast<milliseconds>(t2 - t1).count() << std::endl;

  auto context = new ParseContext();
  context->json_text = json_text_u16;
  napi_value promise;
  napi_create_promise(env, &context->deferred, &promise);
  napi_value resource_name;
  napi_create_string_utf8(env, "json-async-worker", strlen("json-async-worker"), &resource_name);
  napi_create_async_work(env, nullptr, resource_name, parseRun, parseComplete, context, &context->work);
  napi_queue_async_work(env, context->work);
  return promise;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc = {
      "parseAsync",
      nullptr,
      Parse,
      nullptr,
      nullptr,
      nullptr,
      static_cast<napi_property_attributes>(napi_writable | napi_enumerable | napi_configurable),
      nullptr};
  napi_define_properties(env, exports, 1, &desc);
  return exports;
}

NAPI_MODULE(addon, Init)
