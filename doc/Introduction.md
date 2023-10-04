# Background (The IT industry's *JSON* problem)

JSON has become the de-facto standard for automated exchange of data. Just like everything else in the JavaScript world, it was never designed by a committee. It simply came out of nowhere and slowly replaced what many purists considered to be a superior technology (XML) by extreme simplicity of use and the fact that was so well built-in in JavaScript *(JSON goes brrrr)*.

Today, JSON parsing has become a major problem in computing - and many servers spend huge amounts of CPU time doing it.

The built-in JSON implementation of Node.js/V8 is remarkably well-optimized and it is absolutely impossible to outperform since it can create the resulting object from inside V8 in a very efficient manner.

And it is exactly this creation of the resulting object that is also its major fault. JavaScript being monothreaded, all objects must be created on the main thread while blocking the event loop.

This means that every time your backend application must import a `xxx` MB JSON, everything else - and this means *everything else* - accepting new connections, serving other data, emptying network buffers, running timers - must stop and wait.

# The existing alternatives

## [`simdjson`](https://github.com/simdjson/simdjson) and its official Node.js bindings [`simdjson_nodejs`](https://github.com/luizperes/simdjson_nodejs)

`simdjson` is a remarkably complex JSON parser that tries to take advantage of the AVX512 SIMD instruction set. It claims a throughput of up to 6 GB/s on a 2.4 GHz CPU. It is currently the fastest JSON parser by far. The official Node.js bindings offer synchronous access to its parser and are the first main source of inspiration for this project.

The Node.js bindings have several modes of operation:

*   parsing JSON to check validity - which is usually slightly slower than `JSON.parse` for small files and faster for larger files
*   parsing JSON and creating a native JS object - in which case it is always much slower than the builtin `JSON.parse`
*   parsing JSON and offering a special API to retrieving the data - which is usually slightly slower than `JSON.parse` for small files and faster for larger files

## [`yieldable-json`](https://github.com/ibmruntimes/yieldable-json)

`yieldable-json` is the other source of inspiration for this project. It is a pure JavaScript implementation that is also browser compatible. It parses JSON and yields the CPU to the event loop every 5ms. This allows other waiting tasks to continue executing without (too much) delay. Being well-mannered comes at a 5 times slower than `JSON.parse` cost.

# `everything-json`

`everything-json` tries to combine everything at the price of dropping browser support.

It provides a number of different interfaces:

*   `JSONAsync.parse()` parses on the main thread returning an object with a special API. It is comparable to `simdjson.lazyParse`.

*   `JSONAsync.parseAsync()` parses in a background thread returning an object with a special API. It allows for near-zero latency JSON processing.

*   `JSONAsync.parse().toObject()` permits to convert any sub-tree of the main document to a native JavaScript object - blocking the event loop just like the built-in parser - it is slower than the built-in parser but it allows to convert only a sub-tree.

*   `JSONAsync.parseAsync().toObjectAsync()` permits to convert any sub-tree of the main document to a native JavaScript object - while yielding the CPU just like `yieldable-json` - but it is about 5x times faster and it also allows to convert only a sub-tree.
