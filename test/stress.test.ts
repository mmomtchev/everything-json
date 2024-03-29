import * as fs from 'fs';
import * as path from 'path';
import { assert } from 'chai';
import Piscina from 'piscina';

import { JSON as JSONAsync } from 'everything-json';

describe('stress', function () {
  this.timeout(60000);
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

  it('ensure the object store handles dying objects', function (done) {
    (async function () {
      const json = await JSONAsync.parseAsync(texts[0]);
      const expected = await JSONAsync.parseAsync(texts[0]);

      async function getRandomElement(json: JSONAsync) {
        let current = json;
        let path = '';
        do {
          const allKeys = Object.keys(current.get());
          if (allKeys.length === 0)
            break;
          const key = allKeys[Math.floor(Math.random() * allKeys.length)];
          path += `/${key}`;
          current = current.get()[key];
        } while (current.type === 'array' || current.type === 'object');

        assert.strictEqual(json.path(path), current);
        assert.notEqual(json.path(path), expected.path(path));
        if (typeof json.path(path).get() !== 'object') {
          assert.strictEqual(json.path(path).get(), current.get());
          assert.strictEqual(current.get(), expected.path(path).get());
        } else {
          assert.deepEqual(json.path(path).get(), current.get());
          assert.deepEqual(current.get(), expected.path(path).get());
        }
        return current.get();
      }

      for (let i = 0; i < 1e5; i++) {
        await getRandomElement(json);
      }
    })()
      .then(done)
      .catch(done);
  });

  it('stress test in multiple worker_thread', (done) => {
    const piscina = new Piscina({filename: path.resolve(__dirname, 'worker.mjs')});

    const q: Promise<void>[] = [];
    for (let i = 0; i < 4; i++) {
      q.push(piscina.run(undefined).then((r: any[]) => {
        for (let j = 0; j < texts.length * 2; j++) {
          assert.deepEqual(r[j], expected[j % texts.length]);
        }
      }));
    }

    Promise.all(q).then(() => done()).catch(done);
  });
});
