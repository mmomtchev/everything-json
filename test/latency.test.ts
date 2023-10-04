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

  // We start counting the elapsed time
  // and we launch a regular tick every 10ms
  const start = Date.now();
  let ticks = 0;
  const timer = setInterval(() => void ticks++, 10);
  JSONAsync.latency = 1;

  JSONAsync.parseAsync<FeatureCollection>(jsonText)
    .then((jsonBinary) => {
      const elapsed = Date.now() - start;
      // Manpower losses of up to 25% do not have to be reported to higher command
      assert.isAtLeast(ticks, elapsed / 10 * 0.75);

      return jsonBinary.toObjectAsync();
    })
    .then((geojson) => {
      assert.strictEqual(geojson.type, 'FeatureCollection');
      assert.isArray(geojson.features);

      const elapsed = Date.now() - start;
      assert.isAtLeast(ticks, elapsed / 10 * 0.75);
    })
    .catch(done)
    .then(() => {
      JSONAsync.latency = 5;
      clearInterval(timer);
      done();
    });
});
