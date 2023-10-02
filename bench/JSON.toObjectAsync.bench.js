const fs = require('fs');
const path = require('path');
const b = require('benny');
const { assert } = require('chai');

const yieldable = require('yieldable-json');
const JSONAsync = require('..').JSON;

const testDataPath = path.resolve(__dirname, '..', 'test', 'data');
const testJSON = fs.readFileSync(path.join(testDataPath, 'canada.json'), 'utf-8');

module.exports = b.suite(
  'Asynchronous JSON parsing, converting the data to a native JS object',

  b.add('built-in JSON.parse (synchronous parsing)', async () => {
    const document = JSON.parse(testJSON);
    const data = document.features[0].geometry.coordinates[0][0];
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),
  b.add('yieldable-json (yieldable parsing on the main thread)', async () => {
    const document = await new Promise((resolve, reject) => {
      try {
        yieldable.parseAsync(testJSON, (err, result) => {
          if (err) reject (err);
          resolve(result);
        });
      } catch (err) {
        reject(err);
      }
    });
    const data = document.features[0].geometry.coordinates[0][0];
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),
  b.add('json-async (background parsing with yieldable object construction)', async () => {
    const document = await (await JSONAsync.parseAsync(testJSON)).toObjectAsync();
    const data = document.features[0].geometry.coordinates[0][0];
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),

  b.cycle(),
  b.complete()
);
