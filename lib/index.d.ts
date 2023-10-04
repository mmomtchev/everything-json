/**
 * A binary representation of a JSON element
 */
export class JSON<T> {
  constructor();

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
   * Retrieve a subtree out of the binary JSON object
   * 
   * @returns {(JSON | string | boolean | number | null) [] | Record<string, JSON | string | boolean | number | null>}
   */
  get(): T extends Record<string | number, any> ? {
    [P in keyof T]: T[P] extends Record<string | number, any> ? JSON<T[P]> : T[P];
  } : T;

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
  toObject(): any;

  /**
   * Converts the binary representation to a JS object.
   * 
   * Uses the main thread, but periodically yields the CPU
   * to allow other tasks to run.
   * Allows to convert only a small subtree out of a larger
   * document.
   * 
   * @returns {Promise<any>}
   */
  toObjectAsync(): Promise<any>;

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
   * The currently used simdjson version
   * 
   * @property {string}
   */
  static readonly simdjson_version: string;

  /**
   * The currently used SIMD implementation
   * 
   * @property {string}
   */
  static readonly simd: 'icelake' | 'haswell' | 'westmere' | 'arm64' | 'ppc64' | 'fallback';
}
