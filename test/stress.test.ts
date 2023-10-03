import * as fs from 'fs';
import * as path from 'path';
import { assert } from 'chai';

import { JSON as JSONAsync } from 'everything-json';

describe('stress', () => {
  const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
  const expected = JSON.parse(text);

  it('ensure that the GC does not free the string', () => {
    let document;
    for (let i = 0; i < 50; i++)
      setImmediate(() => {
        document = JSONAsync.parse(text);
      });
  });

  it('ensure that the deferred is not destroyed too early', function(done) {
    this.timeout(20000);
    (async function () {
      for (let i = 0; i < 25; i++) {
        const document = await (await JSONAsync.parseAsync(text)).toObjectAsync() as Record<string, any>;
        const data = document.features[0].geometry.coordinates[0][0];
        assert.isArray(data);
        assert.closeTo(data[0], -65.614, 1e-3);
        assert.closeTo(data[1], 43.42, 1e-3);
        assert.deepEqual(document, expected);
      }
    })()
      .then(done)
      .catch(done);
  });
});
