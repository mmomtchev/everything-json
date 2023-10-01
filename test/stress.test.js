const fs = require('fs');
const path = require('path');
const { assert } = require('chai');

const { JSON } = require('..');

const data = fs.readFileSync('test/data/canada.json', 'utf-8');

describe('stress', () => {
  it('ensure that the GC does not free the string', () => {
    let document;
    for (let i = 0; i < 100; i++)
      setImmediate(() => {
        document = JSON.parse(data);
      });
  });
});
