const b = require('benny');

const yieldable = require('yieldable-json');
const JSONAsync = require('..').JSON;

module.exports = (test, get, utf8, buf) => b.suite(
  'Asynchronous JSON parsing, converting the data to a native JS object',

  b.add('built-in JSON.parse (synchronous parsing)', async () => {
    const document = JSON.parse(utf8);
    const data = get.object(document);
    test(data);
  }),
  b.add('yieldable-json (yieldable parsing on the main thread)', async () => {
    const document = await new Promise((resolve, reject) => {
      try {
        yieldable.parseAsync(utf8, (err, result) => {
          if (err) reject (err);
          resolve(result);
        });
      } catch (err) {
        reject(err);
      }
    });
    const data = get.object(document);
    test(data);
  }),
  b.add('everything-json (background parsing with yieldable object construction) / UTF8', async () => {
    const document = await (await JSONAsync.parseAsync(utf8)).toObjectAsync();
    const data = get.object(document);
    test(data);
  }),
  b.add('everything-json (background parsing with yieldable object construction) / Buffer', async () => {
    const document = await (await JSONAsync.parseAsync(buf)).toObjectAsync();
    const data = get.object(document);
    test(data);
  }),

  b.cycle(),
  b.complete()
);
