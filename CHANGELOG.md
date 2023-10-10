# Changelog

# [1.0.0]

 - Implement an object store keeping weak references to all objects returned to JavaScript and reusing them when possible
 - In TypeScript, return `JSON<any>` for `JSON.path()` when the string cannot be parsed by the type system instead of `never`
 - Allow the suppression of the exception in `.path()`

### [0.9.4] 2023-10-09

 - A new method `.path(rfc6901: string)` allows to directly retrieve a deeply nested JSON element using an RFC6901 path
 - A new method `.proxify()` allows the creation of a `Proxy` object that gives the illusion of working with a real native JS object
 - A new getter `.type` allows to identify the type of the underlying JSON element

### [0.9.3] 2023-10-04

 - A new method `.expand()` returns primitive values as JavaScript values instead of a `JSON` object with a single primitive value
 - Support automatic carrying over of complex TypeScript types via generics
 - Fix [#7](https://github.com/mmomtchev/everything-json/issues/7), handle exceptions in synchronous mode

### [0.9.2] 2023-10-03

- Add an `exports` object to `package.json` for ES6 compatibility

### [0.9.1] 2023-10-03

- Fix `node-pre-gyp` automatic retrieval/build of binaries

## [0.9.0] 2023-10-03

- First alpha release
