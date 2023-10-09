import * as fs from 'fs';
import * as path from 'path';
import * as zlib from 'zlib';
import { assert } from 'chai';
import type { FeatureCollection } from 'geojson';

import { JSON as JSONAsync } from 'everything-json';

const jsonText = zlib.unzipSync(fs.readFileSync(path.resolve(__dirname, 'data', 'sf_citylots.json.gz')));

it('toObjectAsync() yields the CPU', function (done) {
  this.timeout(30000);

  // We start counting the elapsed time
  // and we launch a regular tick every 10ms
  let ticks = 0;
  const timer = setInterval(() => void ticks++, 10);
  JSONAsync.latency = 1;

  let start = Date.now();
  JSONAsync.parseAsync<FeatureCollection>(jsonText)
    .then((jsonBinary) => {
      const elapsed = Date.now() - start;
      console.log(`.parseAsync() latency: ${ticks} ticks for ${elapsed}ms, ${(100 * ticks / (elapsed / 10)).toFixed(2)}% passed`);

      start = Date.now();
      ticks = 0;
      return jsonBinary.toObjectAsync();
    })
    .then((geojson) => {
      assert.strictEqual(geojson.type, 'FeatureCollection');
      assert.isArray(geojson.features);

      const elapsed = Date.now() - start;
      console.log(`.toObjectAsync() latency: ${ticks} ticks for ${elapsed}ms, ${(100 * ticks / (elapsed / 10)).toFixed(2)}% passed`);
      done();
    })
    .catch(done)
    .then(() => {
      JSONAsync.latency = 5;
      clearInterval(timer);
    });
});
