# json-async

An asynchronous alternative to the built-in Node.js/V8 JSON parser

# Background (The IT industry's *JSON* problem)

JSON has become the de-facto standard for automated exchange of data. Just like everything else in the JavaScript world, it was never designed by a committee. It simply came out of nowhere and slowly replaced what many purists considered to be a superior technology (XML) by extreme simplicity of use and the fact that was so well built-in in JavaScript.

Today, JSON parsing has become a major problem in computing - and many servers spend huge amounts of CPU time doing it.

The built-in JSON implementation of Node.js/V8 is remarkably well-optimized and it is absolutely impossible to outperform since it can create the resulting object from inside V8 in a very efficient manner.

And it is exactly this creation of the resulting object that is also its major fault. JavaScript being monothreaded, all objects must be created on the main thread while blocking the event loop.

This means that every time your backend application must import a `xxx` MB JSON, everything else - and this means *everything else* - accepting new connections, serving other data, emptying network buffers, running timers - must stop and wait.

# The existing alternatives

## [`simdjson`](https://github.com/simdjson/simdjson) and its official Node.js bindings [`simdjson_nodejs`](https://github.com/luizperes/simdjson_nodejs)

`simdjson` is a remarkably complex JSON parser that tries to take advantage of the AVX512 SIMD instruction set. It claims a throughput of up to 6 GB/s on a 2.4 GHz CPU. It is currently the fastest JSON parser by far. The official Node.js bindings offer synchronous access to its parser and are the first main source of inspiration for this project.

The Node.js bindings have several modes of operation

*   parsing JSON to check validity - which is usually slightly slower than `JSON.parse` for small files and faster for larger files
*   parsing JSON and creating a native JS object - in which case it is always much slower than the builtin `JSON.parse`
*   parsing JSON and offering a special API to retrieving the data - which is usually slightly slower than `JSON.parse` for small files and faster for larger files

## [`yiedable-json`](https://github.com/ibmruntimes/yieldable-json)

`yiedable-json` is the other source of inspiration for this project. It is a pure JavaScript implementation that is also browser compatible. It parses JSON and yields the CPU to the event loop every 5ms. This allows other waiting tasks to continue executing without (too much) delay. Being well-mannered comes at a 5 times slower than `JSON.parse` cost.

# `json-async`

`json-async` tries to combine everything at the price of dropping browser support.

It provides a number of different interfaces:

*   `JSONAsync.parse()` parses on the main thread returning an object with a special API. It is comparable to `simdjson.lazyParse`.

    *(currently it appears slightly slower because `node-addon-api` does not free the memory in a completely synchronous context forcing the kernel to allocate new pages that must be cleared - in reality it is slightly faster)*

*   `JSONAsync.parseAsync()` parses in a background thread returning an object with a special API. It allows for near-zero latency JSON processing.

*   `JSONAsync.parse().toObject()` permits to convert any sub-tree of the main document to a native JavaScript object - blocking the event loop just like the built-in parser - it is slower than the built-in parser but it allows to convert only a sub-tree.

*   `JSONAsync.parseAsync().toObjectAsync()` permits to convert any sub-tree of the main document to a native JavaScript object - while yielding the CPU just like `yieldable-json` - but it is about 5x times faster and it also allows to convert only a sub-tree.

# Usage

Drilling down the document with `.get()` returns a single indirection level converted to JavaScript - array of `JSON` elements, object of `JSON` elements or primitive values. It is almost free of event loop latency. Using `.toObject()` converts the remaining subtree to JavaScript - which can incur an event loop latency if the tree is large. Using `.toObjectAsync()` does not block the event loop, but it is much slower.

## Sync mode

These two examples convert a subtree of the main document to a JS object.

```ts
import { JSON } from 'json-async';
const fs = require('fs');

const document = JSON.parse(fs.readFileSync('test/data/canada.json'));
// With the built-in JSON parser, this would have been equivalent to
// console.log(document.features[0].geometry.coordinates[10])
console.log(document.get().features.get()[0].get().geometry.get()
  .coordinates.get()[10].toObject());
```

## Async mode

```ts
import { JSON } from 'json-async';
const fs = require('fs');

const document = await JSON.parseAsync(
  await fs.promises.readFile('test/data/canada.json'));

console.log(await document.get().features.get()[0].get().geometry.get()
  .coordinates.get()[10].toObjectAsync());
```

Be aware that not only reading a file as a `Buffer` is about 3 times faster than reading a file in a `string` with UTF8 encoding, it also avoids a second UTF decoding pass when parsing the JSON data.

# Current status

Early alpha, unpublished, highly experimental

# Full API

<!-- Generated by documentation.js. Update this documentation by updating the source code. -->

### Table of Contents

*   [JSON](#json)
    *   [get](#get)
    *   [toObject](#toobject)
    *   [toObjectAsync](#toobjectasync)
    *   [parse](#parse)
        *   [Parameters](#parameters)
    *   [parseAsync](#parseasync)
        *   [Parameters](#parameters-1)

## JSON

A binary representation of a JSON element

### get

Retrieve a subtree out of the binary JSON object

Returns **([string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String) | [boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean) | [number](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number) | [Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)<[JSON](#json)> | Record<[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String), [JSON](#json)>)**&#x20;

### toObject

Converts the binary representation to a JS object.

Will block the event loop while the conversion is running.
Significantly slower than the built-in JSON parser but
allows to convert only a small subtree out of a larger
document.

Returns **any**&#x20;

### toObjectAsync

Converts the binary representation to a JS object.

Uses the main thread, but periodically yields the CPU
to allow other tasks to run.
Allows to convert only a small subtree out of a larger
document.

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)\<any>**&#x20;

### parse

Parse a string and return its binary representation.

Will block the event loop while it parses the JSON. Slightly
slower for small files but faster for larger files compared
to the built-in JSON parser.

#### Parameters

*   `text` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** JSON to parse

Returns **[JSON](#json)**&#x20;

### parseAsync

Parse a string and return its binary representation.

Unlike the built-in JSON parser, it does not incur any latency
on the event loop while it parses the JSON. Slightly slower for
small files but faster for larger files compared to the built-in
JSON parser.

#### Parameters

*   `text` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** JSON to parse

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)<[JSON](#json)>**&#x20;
