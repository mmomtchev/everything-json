import * as fs from 'fs';
import * as path from 'path';
import { assert } from 'chai';

import { JSON as JSONAsync } from 'everything-json';

describe('stress', function () {
  this.timeout(20000);
  const dir = path.resolve(__dirname, 'data');
  const files = fs.readdirSync(dir);
  const texts = files.filter((file) => file.match(/json$/)).map((file) => fs.readFileSync(path.resolve(dir, file), 'utf-8'));
  const expected = texts.map((text) => JSON.parse(text));

  it('ensure that the GC does not free the string', function (done) {
    let completed = 0;
    for (let i = 0; i < texts.length * 2; i++) {
      const idx = i % texts.length;
      setImmediate(async () => {
        try {
          const document = JSONAsync.parse(texts[idx]).toObject();
          assert.deepEqual(document, expected[idx]);
          completed++;
          if (completed === texts.length * 2)
            done();
        } catch (e) {
          done(new Error(`${files[idx]} : ${e}`));
        }
      });
    }
  });

  it('ensure that the deferred is not destroyed too early', function (done) {
    (async function () {
      const q: Promise<void>[] = [];
      for (let i = 0; i < texts.length * 2; i++) {
        const idx = i % texts.length;
        q.push(JSONAsync.parseAsync(texts[idx])
          .then((json) => json.toObjectAsync())
          .then((document) => {
            assert.deepEqual(document, expected[idx]);
          }));
      }
      await Promise.all(q);
    })()
      .then(done)
      .catch(done);
  });
});
