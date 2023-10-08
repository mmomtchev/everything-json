export type JSONType<T> = T extends Array<any> ? 'array' :
  T extends Record<string, any> ? 'object' :
  T extends string ? 'string' :
  T extends number ? 'number' :
  T extends boolean ? 'boolean' :
  T extends null ? 'null' :
  'array' | 'object' | 'string' | 'number' | 'boolean' | 'null';

export type JSONProxy<T> = T extends Record<string | number, any> ? {
  [P in keyof T]: JSONProxy<T[P]>;
} & {
  [JSON.symbolToObject]: () => T;
  [JSON.symbolToObjectAsync]: () => Promise<T>;
  [JSON.symbolType]: JSONType<T>;
} : T;

export type RFC6901<T extends Record<string | number, any>, PATH extends string> =
  PATH extends `/${infer PROP}/${infer SUB}` ?
  RFC6901<T[PROP], `/${SUB}`> :
  PATH extends `/${infer PROP}` ?
  JSON<T[PROP]> :
  never;

/**
 * A binary representation of a JSON element
 */
export class JSON<T = any> {
  constructor();

  /**
   * The underlying type of the JSON element
   * 
   * @type {'object' | 'array' | 'string' | 'number' | 'boolean' | 'null'}
   */
  type: JSONType<T>;

  /**
   * Parse a string and return its binary representation.
   * 
   * Will block the event loop while it parses the JSON. Slightly
   * slower for small files but faster for larger files compared
   * to the built-in JSON parser.
   * 
   * @param {string} text JSON to parse
   * @returns {JSON}
   */
  static parse<U = any>(text: string | Buffer): JSON<U>;

  /**
   * Parse a string and return its binary representation.
   * 
   * Unlike the built-in JSON parser, it does not incur any latency
   * on the event loop while it parses the JSON. Slightly slower for
   * small files but faster for larger files compared to the built-in
   * JSON parser.
   * 
   * @param {string} text JSON to parse
   * @returns {Promise<JSON>}
   */
  static parseAsync<U = any>(text: string | Buffer): Promise<JSON<U>>;

  /**
   * Retrieve a subtree out of the binary JSON object.
   * 
   * @returns {string | boolean | number | null | Array<JSON> | Record<string, JSON>}
   */
  get(): T extends Record<string | number, any> ? {
    [P in keyof T]: JSON<T[P]>;
  } : T;

  /**
   * Retrieve a subtree out of the binary JSON object
   * automatically expanding primitive values.
   * 
   * @returns {(JSON | string | boolean | number | null) [] | Record<string, JSON | string | boolean | number | null> | string | boolean | number | null}
   */
  expand(): T extends Record<string | number, any> ? {
    [P in keyof T]: T[P] extends Record<string | number, any> ? JSON<T[P]> : T[P];
  } : T;

  /**
   * Retrieves a deeply nested JSON element referenced by the RFC6901 JSON pointer.
   * 
   * This is much faster than recursing down with .get()/.expand() but
   * it will still have an O(n) complexity relative to the arrays and objects
   * sizes since simdjson stores arrays and objects as lists.
   * 
   * @returns {any}
   */
  path<PATH extends string>(rfc6901: PATH): T extends Record<string | number, any> ? RFC6901<T, PATH> : never;

  /**
   * Converts the binary representation to a JS object.
   * 
   * Will block the event loop while the conversion is running.
   * Significantly slower than the built-in JSON parser but
   * allows to convert only a small subtree out of a larger
   * document.
   * 
   * @returns {any}
   */
  toObject(): T;

  /**
   * Converts the binary representation to a JS object.
   * 
   * Uses the main thread, but periodically yields the CPU
   * to allow other tasks to run.
   * 
   * Allows to convert only a small subtree out of a larger
   * document.
   * 
   * @returns {Promise<any>}
   */
  toObjectAsync(): Promise<T>;

  /**
   * Creates a Proxy object that gives the illusion of a real object.
   * 
   * This is an instantaneous zero-latency method for creating a
   * `Proxy` object that works (almost) like a real object but
   * calls .expand() when needs to retrieve a property.
   * 
   * @returns {any}
   */
  proxify(): JSONProxy<T>;

  /**
   * Allows to change the default latency limit.
   * 
   * CPU will be yielded every `latency` milliseconds.
   * 
   * @property {number}
   * @default 5
   */
  static latency: number;

  /**
   * The currently used simdjson version.
   * 
   * @property {string}
   */
  static readonly simdjson_version: string;

  /**
   * The currently used SIMD implementation.
   * 
   * @property {string}
   */
  static readonly simd: 'icelake' | 'haswell' | 'westmere' | 'arm64' | 'ppc64' | 'fallback';

  /**
   * Symbol.toObject to be used for Proxies
   */
  static readonly symbolToObject: unique symbol;

  /**
   * Symbol.toObjectAsync to be used for Proxies
   */
  static readonly symbolToObjectAsync: unique symbol;

  /**
   * Symbol.type to be used for Proxies
   */
  static readonly symbolType: unique symbol;
}
