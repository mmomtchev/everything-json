import * as fs from 'fs';
import * as path from 'path';
import * as zlib from 'zlib';
import { assert } from 'chai';
import type { FeatureCollection } from 'geojson';

import { JSON as JSONAsync } from 'everything-json';

const jsonText = zlib.unzipSync(fs.readFileSync(path.resolve(__dirname, 'data', 'sf_citylots.json.gz')));

it('toObjectAsync() yields the CPU', function (done) {
  this.timeout(30000);
  this.retries(3);
  // @ts-ignore
  if (JSONAsync.debug) this.skip();

  // We start counting the elapsed time
  // and we launch a regular tick every 10ms
  const start = Date.now();
  let ticks = 0;
  let phase1ticks: number;
  const timer = setInterval(() => void ticks++, 10);
  JSONAsync.latency = 1;

  JSONAsync.parseAsync<FeatureCollection>(jsonText)
    .then((jsonBinary) => {
      const elapsed = Date.now() - start;
      // Accept up to 50% losses
      assert.isAtLeast(ticks, elapsed / 10 * 0.5);
      phase1ticks = ticks;

      return jsonBinary.toObjectAsync();
    })
    .then((geojson) => {
      assert.strictEqual(geojson.type, 'FeatureCollection');
      assert.isArray(geojson.features);

      const elapsed = Date.now() - start;
      assert.isAtLeast(ticks, elapsed / 10 * 0.5);
      assert.isAbove(ticks, phase1ticks);
      done();
    })
    .catch(done)
    .then(() => {
      JSONAsync.latency = 5;
      clearInterval(timer);
    });
});
