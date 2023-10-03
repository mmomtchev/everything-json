const fs = require('fs').promises;
const path = require('path');
const b = require('benny');
const { assert } = require('chai');

const testDataPath = path.resolve(__dirname, '..', 'test', 'data', 'canada.json');

module.exports = b.suite(
  'Reading a file',

  b.add('readFile (UTF8)', async () => {
    const data = await fs.readFile(testDataPath, 'utf-8');
    assert.isString(data);
  }),
  b.add('readFile (Buffer)', async () => {
    const data = await fs.readFile(testDataPath);
    assert.instanceOf(data, Buffer);
  }),

  b.cycle(),
  b.complete()
);
