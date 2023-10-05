const b = require('benny');

const simdjson = require('simdjson');
const JSONAsync = require('..').JSON;

module.exports = (test, get, utf8, buf) => b.suite(
  'Synchronous JSON parsing with creation of a full native JS object',

  b.add('built-in JSON.parse', () => {
    const document = JSON.parse(utf8);
    const data = get.object(document);
    test(data);
  }),
  // Using an async function (even when in sync mode) gives
  // node-addon-api a chance to free its buffers
  // See https://github.com/nodejs/node-addon-api/issues/1140
  b.add('everything-json / UTF8', async () => {
    const document = JSONAsync.parse(utf8).toObject();
    const data = get.object(document);
    test(data);
  }),
  b.add('everything-json / Buffer', async () => {
    const document = JSONAsync.parse(buf).toObject();
    const data = get.object(document);
    test(data);
  }),
  b.add('simdjson', () => {
    const document = simdjson.parse(utf8);
    const data = get.object(document);
    test(data);
  }),

  b.cycle(),
  b.complete()
);
