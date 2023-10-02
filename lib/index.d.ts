/**
 * A binary representation of a JSON element
 */
export class JSON {
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
  static parse(text: string): JSON;

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
  static parseAsync(text: string): Promise<JSON>;

  /**
   * Retrieve a subtree out of the binary JSON object
   * 
   * @returns {string | boolean | number | JSON[] | Record<string, JSON>}
   */
  get(): any;

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
}
