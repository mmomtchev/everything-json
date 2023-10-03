const fs = require('fs');
const path = require('path');
const b = require('benny');
const { assert } = require('chai');

const simdjson = require('simdjson');
const JSONAsync = require('..').JSON;
const yieldable = require('yieldable-json');

const testDataPath = path.resolve(__dirname, '..', 'test', 'data');
const testJSON = fs.readFileSync(path.join(testDataPath, 'canada.json'), 'utf-8');

module.exports = b.suite(
  'Asynchronous JSON parsing, accessing 1 element through each API\'s fastest interface',

  b.add('built-in JSON.parse (synchronous parsing)', async () => {
    const document = JSON.parse(testJSON);
    const data = document.features[0].geometry.coordinates[0][0];
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),
  b.add('json-async (100% background processing with zero latency)', async () => {
    const document = await JSONAsync.parseAsync(testJSON);
    const data = document.get().features.get()[0].get().geometry.get().coordinates.get()[0].get()[0].toObject();
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),
  b.add('simdjson (synchronous parsing)', () => {
    const document = simdjson.lazyParse(testJSON);
    const data = document.valueForKeyPath('features[0].geometry.coordinates[0][0]');
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),
  b.add('yieldable-json (asynchronous parsing on the main thread by yielding)', async () => {
    const document = await new Promise((resolve, reject) => {
      try {
        yieldable.parseAsync(testJSON, (err, result) => {
          if (err) reject(err);
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


  b.cycle(),
  b.complete()
);
