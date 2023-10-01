const fs = require('fs');
const path = require('path');
const b = require('benny');
const { assert } = require('chai');

const simdjson = require('simdjson');
const JSONAsync = require('..').JSON;

const testDataPath = path.resolve(__dirname, '..', 'test', 'data');
const testJSON = fs.readFileSync(path.join(testDataPath, 'canada.json'), 'utf-8');

module.exports = b.suite(
  'Asynchronous JSON parsing, converting the data to a native JS object',

  b.add('built-in JSON.parse', async () => {
    const document = JSON.parse(testJSON);
    const data = document.features[0].geometry.coordinates[0][0];
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),
  b.add('json-async', async () => {
    const document = await (await JSONAsync.parseAsync(testJSON)).toObjectAsync();
    const data = document.features[0].geometry.coordinates[0][0];
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),
  b.add('simdjson', () => {
    const document = simdjson.parse(testJSON);
    const data = document.features[0].geometry.coordinates[0][0];
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  }),

  b.cycle(),
  b.complete()
);
