import * as fs from 'fs';
import * as path from 'path';
import { assert } from 'chai';
import type { FeatureCollection, Polygon } from 'geojson';

import { JSON as JSONAsync, AddToObject } from 'everything-json';

describe('proxify()', () => {
  const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
  const expected = JSON.parse(text);

  it('parse()', () => {
    const document = JSONAsync.parse<FeatureCollection>(text).proxify();
    assert.isObject(document);
    assert.sameMembers(Object.keys(document), ['type', 'features']);
    const features = document.features;
    assert.isArray(features);
    assert.closeTo((features[0].geometry as Polygon).coordinates[10][2][0], -55.946, 1e-3);
    assert.closeTo((features[0].geometry as Polygon).coordinates[10][2][0], -55.946, 1e-3);
    assert.deepEqual((features[0].geometry as AddToObject<Polygon>).coordinates[10].toObject(),
      (expected.features[0].geometry as Polygon).coordinates[10]);
  });

  it('parseAsync()', (done) => {
    JSONAsync.parseAsync<FeatureCollection>(text)
      .then((raw) => {
        const document = raw.proxify();
        assert.isObject(document);
        assert.sameMembers(Object.keys(document), ['type', 'features']);
        const features = document.features;
        assert.isArray(features);
        assert.deepEqual(features, expected.features);
        done();
      })
      .catch(done);
  });
});
