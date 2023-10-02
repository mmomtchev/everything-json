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

  it('ensure that the deferred is not destroyed too early', () => {
    (async function () {
      for (let i = 0; i < 100; i++) {
        const document = await (await JSONAsync.parseAsync(data)).toObjectAsync();
        const data = document.features[0].geometry.coordinates[0][0];
        assert.isArray(data);
        assert.closeTo(data[0], -65.614, 1e-3);
        assert.closeTo(data[1], 43.42, 1e-3);
      }
    })();
  })
});
