<!-- Generated by documentation.js. Update this documentation by updating the source code. -->

### Table of Contents

*   [JSON][1]
    *   [get][2]
    *   [toObject][3]
    *   [toObjectAsync][4]
    *   [parse][5]
        *   [Parameters][6]
    *   [parseAsync][7]
        *   [Parameters][8]
    *   [latency][9]
    *   [simdjson\_version][10]
    *   [simd][11]

## JSON

A binary representation of a JSON element

### get

Retrieve a subtree out of the binary JSON object

Returns **([string][12] | [boolean][13] | [number][14] | [Array][15]<[JSON][1]> | Record<[string][12], [JSON][1]>)**&#x20;

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

Returns **[Promise][16]\<any>**&#x20;

### parse

Parse a string and return its binary representation.

Will block the event loop while it parses the JSON. Slightly
slower for small files but faster for larger files compared
to the built-in JSON parser.

#### Parameters

*   `text` **[string][12]** JSON to parse

Returns **[JSON][1]**&#x20;

### parseAsync

Parse a string and return its binary representation.

Unlike the built-in JSON parser, it does not incur any latency
on the event loop while it parses the JSON. Slightly slower for
small files but faster for larger files compared to the built-in
JSON parser.

#### Parameters

*   `text` **[string][12]** JSON to parse

Returns **[Promise][16]<[JSON][1]>**&#x20;

### latency

Allows to change the default latency limit.

CPU will be yielded every `latency` milliseconds.

Type: [number][14]

### simdjson\_version

The currently used simdjson version

Type: [string][12]

### simd

The currently used SIMD implementation

Type: (`"icelake"` | `"haswell"` | `"westmere"` | `"arm64"` | `"ppc64"` | `"fallback"`)

[1]: #json

[2]: #get

[3]: #toobject

[4]: #toobjectasync

[5]: #parse

[6]: #parameters

[7]: #parseasync

[8]: #parameters-1

[9]: #latency

[10]: #simdjson_version

[11]: #simd

[12]: https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String

[13]: https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean

[14]: https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Number

[15]: https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array

[16]: https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise