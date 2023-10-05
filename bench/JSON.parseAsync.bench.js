const b = require('benny');

const simdjson = require('simdjson');
const JSONAsync = require('..').JSON;
const yieldable = require('yieldable-json');

module.exports = (test, get, utf8, buf) => b.suite(
  'Asynchronous JSON parsing, accessing 1 element through each API\'s fastest interface',

  b.add('built-in JSON.parse (synchronous parsing)', async () => {
    const document = JSON.parse(utf8);
    const data = get.object(document);
    test(data);
  }),
  b.add('everything-json (100% background processing with zero latency) from UTF8 string', async () => {
    const document = await JSONAsync.parseAsync(utf8);
    const data = get.everything(document);
    test(data);
  }),
  b.add('everything-json (100% background processing with zero latency) from Buffer', async () => {
    const document = await JSONAsync.parseAsync(buf);
    const data = get.everything(document);
    test(data);
  }),
  b.add('simdjson (synchronous parsing)', () => {
    const document = simdjson.lazyParse(utf8);
    const data = get.simdjson(document);
    test(data);
  }),
  b.add('yieldable-json (asynchronous parsing on the main thread by yielding)', async () => {
    const document = await new Promise((resolve, reject) => {
      try {
        yieldable.parseAsync(utf8, (err, result) => {
          if (err) reject(err);
          resolve(result);
        });
      } catch (err) {
        reject(err);
      }
    });
    const data = get.object(document);
    test(data);
  }),


  b.cycle(),
  b.complete()
);
