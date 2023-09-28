# json-async

An experiment in computer futility

# Background (The IT industry's *JSON* problem)

Recently, when debugging a small and barely noticeable delay when refreshing a website that I manage, I found that it was caused by parsing a relatively big `JSON` file. The rabbit hole ran very deep on that one. I vaguely remembered while working on `gdal-async` some years ago that there was one hugely problematic API call that couldn't be made to work without blocking the event loop - consuming GeoJSON data.

I quickly discovered that I was far from being the first one to be confronted with this astonishing issue in modern IT. As I went on exploring the various solutions people had found, I was stunned to find that there was even a SIMD-enabled library that used the AVX instruction set to speed up JSON parsing. As SIMD is definitely not the very first thing that comes to mind when thinking about JSON, I went through various research papers and presentations.

*(If you, just like me, are curious about it - there is an AVX instruction that allows to do 32 table lookups at the same time)*

**Turns out the IT industry has a serious JSON problem**

Implementing that library - whose complexity is almost beyond mortal comprehension - was partly motivated by a study of one big web hosting company which found that half of their CPU cycles were spending encoding and decoding JSON. Seriously?

And as if JSON parsing in C++ was not enough, there is JavaScript which makes it even worse. JSON, the ubiquitous data exchange format in the JavaScript world, is also one of its worst weak spots - both for Node.js and for browser JavaScript.

JavaScript engines, being mono-threaded, cannot allow background threads to access their object model. Which means that any JSON encoder or decoder must be (almost) fully synchronous - blocking the main thread until its task is done. Get served an 8MB JSON - you can expect up to 1s of user interaction delay.

It seems that the problem is very widely recognized, because I was stunned by the performance of the built-in parser in V8. In fact, the ultra high performance C++ `simdjson` outperforms it only on bigger files - and does so by a significant margin only on AVX512-enabled CPUs. In fact, the built-in JSON encoder has a performance that is impossible to beat - only constructing the V8 object by using raw C `Node-API` (we are not even talking `node-addon-api` here) calls is about twice slower than parsing the JSON *AND* constructing the resulting object by the builtin parser.

The Node.js version `node-simdjson` is almost always slower than the built-in parser (unless the lazy proxying mode is used - you can read more about it in the package).

It seems also that the problem of the user experiencing delays (or dropped packets on the backend side) has also been recognized because there is also a very interesting JSON implementation by the Node.js core-team called `yieldable-json`. It is a pure JavaScript parser that is about 5 times slower than the built-in parser but it yields the control every 5ms to ensure smooth execution of the whole program.

# `json-async`

This project is an ongoing experiment that tries to combine the best of both worlds at the price of omitting browser support:

* A very fast JSON parser (`simdjson`)
* An asynchronous implementation optimized for JavaScript that does as much as possible in the background before constructing the object on the main thread
* The option to yield the CPU every `x`ms

# Current status

Tests on my 8MB JSON file:

Reading as `utf-8`:

| file read | `JSON` | `node-simdjson` | `yieldable-json` | `json-async` Node-API | `json-async` Raw V8 |
| --- | --- | --- | --- | --- | --- |
| 38ms | 92ms | 284ms | 437ms | 207ms foreground + 360ms background | 170ms foreground + 60ms background |

Reading as `Buffer`:

| file read | `JSON` | `node-simdjson` | `yieldable-json` | `json-async` Node-API | `json-async` Raw V8 |
| --- | --- | --- | --- | --- | --- |
| 6ms | 127ms | N/A | 495ms | 207ms foreground + 300ms background | 170ms foreground + 60ms background |


**Alas, the conclusion of the experiment is that there is no background processing possible that will render the foreground processing faster then the speed of the creation of the object on the main thread - where the main speedup vs `node-simdjson` comes from using the V8 bulk insertion primitives.**
