# `everything-json`

*An asynchronous alternative to the built-in Node.js/V8 JSON parser with everything*

[![ISC](https://img.shields.io/github/license/mmomtchev/everything-json)](https://github.com/mmomtchev/everything-json/blob/main/LICENSE)
[![Node.js CI](https://github.com/mmomtchev/everything-json/actions/workflows/test-dev.yml/badge.svg)](https://github.com/mmomtchev/everything-json/actions/workflows/test-dev.yml)
[![codecov](https://codecov.io/gh/mmomtchev/everything-json/graph/badge.svg?token=NNGNMBL0XN)](https://codecov.io/gh/mmomtchev/everything-json)
[![npm](https://img.shields.io/npm/v/everything-json)](https://www.npmjs.com/package/everything-json)
[![Test npm package](https://github.com/mmomtchev/everything-json/actions/workflows/test-package.yml/badge.svg)](https://github.com/mmomtchev/everything-json/actions/workflows/test-package.yml)
<img src="https://raw.githubusercontent.com/mmomtchev/mmomtchev/refs/heads/master/combat-medal.svg" alt="Combat proven on the backend during a DoS attack" width="25">

![Everything Bagel](https://raw.githubusercontent.com/mmomtchev/everything-json/main/doc/everything-bagel.png)

# The `everything-json` manifesto

Read it [here](https://github.com/mmomtchev/everything-json/blob/main/doc/Introduction.md).

# Usage

`everything-json` is a two-stage JSON parser based on `simdjson`. Its first pass creates a binary representation of the JSON data. This pass is independent of V8 and can be performed asynchronously in a background thread without any effect on the event loop by calling `JSON.parseAsync()` instead of `JSON.parse()`. The resulting object, of `JSON` type, can be recursively decoded using its `.get()` method which returns a single level of indirection or by using its `.toObject()` method which returns the full sub-tree as a native JS object - just like the native `JSON.parse()`.

Due to the limitations of the V8 engine, the second stage - `.get()` / `.expand()` / `.toObject()` / `.toObjectAsync()` can only be performed on the main thread.

`.get()` is usually fast enough - unless dealing with a huge array with millions of elements - and it can be used synchronously without incurring (almost) any latency. `.get()` returns:

    string | boolean | number | null | Array<JSON> | Record<string, JSON>

`.expand()` is like `.get()` but automatically expands all primitive values and returns:

    Array<JSON | string | boolean | number | null> |
    Record<string, JSON | string | boolean | number | null> |
    string | boolean | number | null

`.toObject()` works just like the built-in `JSON.parse()`. It can block the event loop for significant amounts of time. It is slower than the built-in parser but it allows to convert only a subtree of the main document - by first drilling down with `.get()` to reach it.

`.toObjectAsync()` also uses the main thread to create the JavaScript object, but it periodically yields the CPU, allowing the event loop to make one full iteration - executing all pending tasks - before continuing again. It is capable of stopping in the middle of an array or an object, but not in the middle of a string - which should not be a problem unless the string is in the megabytes range. The default period is 5ms and it is configurable by setting `JSON.latency`. `.toObjectAsync()` is similar to `yieldable-json` but it much faster - up to 20 times in some cases, see below.

`.proxify()` allows to create JavaScript `Proxy` that will create the illusion of working with a real object, intercepting requests to retrieve a property and calling `.expand()` behind the scenes. While practical for accessing a few values, this is also substantially slower.

`.path(rfc6901: string)` can retrieve directly a deeply nested JSON element specified by an RFC6901 JSON pointer. This is much faster than recursing down with .get()/.expand() but it will still have an `O(n)` complexity relative to the arrays and objects sizes since `simdjson` stores arrays and objects as lists.

`.type` allows to identify the type of the underlying JSON element: `array | object | string | boolean | number | null`.

If you have a choice, always read the data as a `Buffer` instead of `string` using the `utf-8` argument of `readFile`. It is 3 times faster and it also avoids a second UTF8 decoding pass when parsing the JSON data. `everything-json` supports reading from a `Buffer` if the data is UTF8.

## Sync mode

These two examples convert a subtree of the main document to a JS object.

```ts
import { JSON } from 'everything-json';
import * as fs from 'fs';

const document = JSON.parse(fs.readFileSync('test/data/canada.json'));
// With the built-in JSON parser, this would have been equivalent to
// console.log(document.features[0].geometry.coordinates[10])
console.log(document.get().features.get()[0].get().geometry.get()
  .coordinates.get()[10].toObject());
```

## Async mode

```ts
import { JSON } from 'everything-json';
import * as fs from 'fs';

const document = await JSON.parseAsync(
  await fs.promises.readFile('test/data/canada.json'));

console.log(await document.get().features.get()[0].get().geometry.get()
  .coordinates.get()[10].toObjectAsync());
```

## With RFC6901 `path`

```ts
import { JSON } from 'everything-json';
import * as fs from 'fs';

const document = JSON.parse(fs.readFileSync('test/data/canada.json'));
console.log(document.path('/features/0/geometry/coordinates/10/2/0').get());
```

## With `proxify`

```ts
import { JSON } from 'everything-json';
import * as fs from 'fs';

const document = JSON.parse(fs.readFileSync('test/data/canada.json'));
console.log(document.features.[0].geometry.coordinates[10][2][0]);
```

## With `Next.js`

`everything-json` can be used with `Next.js` - but only for data fetching on the server side. It works particularly well with the new `app` router. Simply import it in your server-side component:

```ts
import { JSON as JSONAsync } from 'everything-json';
```

then add to your `next.config.js`:

```js
// Instruct webpack to leave all references to everything-json
// as external require() statements
export default {
  webpack: (config) => {
    if (config.externals)
      config.externals.push('everything-json');
    else
      config.externals = ['everything-json'];
    return config;
  }
};
```

# Using structured JSON in TypeScript

When used in TypeScript `everything-json` supports carrying over the structure of your document via the use of generics:

```ts
interface S {
  number: number,
  array: number[];
};

const document = JSON.parse<S>(json).get();
// document will have its type correctly deduced as
// JSON<{ number: JSON<number>, array: JSON<number[]> }>
```

This is particularly useful when parsing GeoJSON with the `@types/geojson` package: 
```ts
import type { FeatureCollection } from 'geojson';
const document = JSON.parse<FeatureCollection>(geojson);
```

# The Object Store

`everything-json` has the very special property that allows:

```js
const text = fs.readFileSync(file);
const document = JSONAsync.parse(text);

const root1 = document.get();
const root2 = document.get();
assert.strictEqual(root1, root2);
```

Every time `.get()`/`expand()` returns an object, `everything-json` keep a weak reference that object and it will immediately return it on any subsequent calls - as long as the GC hasn't collected it. This permits for efficient long-term storage of JSON data structures in their binary representation, retrieving individual values as they are needed.

`.toObject()` / `.toObjectAsync()` are excluded from this cache as it is expected that these methods will be used only once. This allows to avoid any extra allocation in these performance-critical functions.

# Full API

Read it [here](https://github.com/mmomtchev/everything-json/blob/main/doc/API.md).

# Security

As with every other software that parses untrusted and unsanitized user input, there is a risk of vulnerability. However as JSON is a very simple format and `simdjson` is an extensively tested and very widely used library, security vulnerabilities are rather unlikely.

# Benchmarks

Some benchmarks on a Haswell CPU *(Number of parsing operations per second.)*

Four tests:
* Synchronous parsing then retrieve 1 element with the fastest API
* Synchronous parsing then convert to JS object
* Asynchronous parsing then retrieve 1 element with the fastest API
* Asynchronous parsing then convert to JS object

![twitter.json](https://raw.githubusercontent.com/mmomtchev/everything-json/main/doc/bench-twitter.png)
![canada.json](https://raw.githubusercontent.com/mmomtchev/everything-json/main/doc/bench-canada.png)

## `twitter.json` : JSON with lots of strings and a deep structure

| / | Builtin `JSON.parse()` | `everything-json` from `Buffer` | `everything-json` from UTF8 string | `simdjson` | `yieldable-json` |
| --- | --- | --- | --- | --- | --- |
| sync then 1 element | 583 | 1399 | 562 | 568 | n/a |
| sync then JS object | 583 | 146 | 125 | 145 | n/a |
| async then 1 element | n/a | 1133 | 523 | n/a | 66 |
| async then JS object | n/a | 142 | 122 | n/a | 66 |

## `canada.json` : GeoJSON (lots of floating-point numbers)

| / | Builtin `JSON.parse()` | `everything-json` from `Buffer` | `everything-json` from UTF8 string | `simdjson` | `yieldable-json` |
| --- | --- | --- | --- | --- | --- |
| sync then 1 element | 68 | 276 | 247 | 241 | n/a |
| sync then JS object | 69 | 26 | 26 | 33 | n/a |
| async then 1 element | n/a | 242 | 227 | n/a | 5 |
| async then JS object | n/a | 25 | 25 | n/a | 5 |

# Is a browser WASM version feasible?

Although possible, it won't match the performance of the native Node.js version and it is not planned at the moment.

The solution would be to have a pre-spawned Web Worker that is going to run an eventual WASM port of `simdjson`. It will store its JSON tape in a `SharedArrayBuffer` which will be read by the main thread.

# Copyright

Copyright 2023 Momtchil Momtchev <momtchil@momtchev.com>

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

[Bagel Vectors by Vecteezy](https://www.vecteezy.com/free-vector/bagel)
