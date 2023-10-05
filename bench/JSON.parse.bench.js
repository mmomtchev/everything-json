const b = require('benny');

const simdjson = require('simdjson');
const JSONAsync = require('..').JSON;

module.exports = (test, get, utf8, buf) => b.suite(
  'Synchronous JSON parsing, accessing 1 element through each API\'s fastest interface',

  b.add('built-in JSON.parse', () => {
    const document = JSON.parse(utf8);
    const data = get.object(document);
    test(data);
  }),
  // Using an async function (even when in sync mode) gives
  // node-addon-api a chance to free its buffers
  // See https://github.com/nodejs/node-addon-api/issues/1140
  b.add('everything-json from UTF8 encoded string', async () => {
    const document = JSONAsync.parse(utf8);
    const data = get.everything(document);
    test(data);
  }),
  b.add('everything-json from Buffer', async () => {
    const document = JSONAsync.parse(buf);
    const data = get.everything(document);
    test(data);
  }),
  b.add('simdjson', () => {
    const document = simdjson.lazyParse(utf8);
    const data = get.simdjson(document);
    test(data);
  }),

  b.cycle(),
  b.complete()
);
