import * as fs from 'fs';
import * as path from 'path';
import { assert } from 'chai';
import type { FeatureCollection, Polygon } from 'geojson';

import { JSON as JSONAsync } from 'everything-json';

describe('from string', () => {
  const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
  const expected = JSON.parse(text);

  it('get()', () => {
    const document = JSONAsync.parse<FeatureCollection>(text);
    assert.isObject(document.get());
    assert.sameMembers(Object.keys(document.get()), ['type', 'features']);
    const features = document.get().features.get();
    assert.isArray(features);
    assert.instanceOf(features[0], JSONAsync);

    assert.closeTo((features[0].get().geometry as JSONAsync<Polygon>)
      .get().coordinates.get()[10].get()[2].get()[0].get(), -55.946, 1e-3);
    assert.closeTo((features[0].get().geometry as JSONAsync<Polygon>)
      .get().coordinates.get()[10].get()[2].expand()[0], -55.946, 1e-3);
  });

  it('toObject()', () => {
    const document = JSONAsync.parse<FeatureCollection>(text);
    const geometry = document.get().features.get()[0].get().geometry.toObject();
    assert.deepEqual(geometry, expected.features[0].geometry);
  });

  it('parseAsync()', (done) => {
    JSONAsync.parseAsync(text)
      .then((document) => {
        assert.isObject(document.get());
        assert.sameMembers(Object.keys(document.get()), ['type', 'features']);
        const features = document.get().features.toObject();
        assert.deepEqual(features, expected.features);
        done();
      })
      .catch(done);
  });

  it('toObjectAsync()', function (done) {
    this.timeout(10000);
    JSONAsync.parseAsync(text)
      .then((document) => document.toObjectAsync())
      .then((object) => {
        assert.isObject(object);
        assert.sameMembers(Object.keys(object), ['type', 'features']);
        assert.closeTo(object.features[0].geometry.coordinates[10][2][0], -55.946, 1e-3);
        assert.deepEqual(object, expected);
        done();
      })
      .catch(done);
  });

  it('parse() throws on invalid JSON', () => {
    assert.throws(() => {
      JSONAsync.parse('invalid JSON');
    }, /TAPE_ERROR: The JSON document has an improper structure/);
  });

  it('parseAsync() throws on invalid JSON', () => {
    JSONAsync.parseAsync('invalid JSON')
      .then(() => {
        throw new Error('did not throw');
      })
      .catch((e) => {
        assert.match(e, /TAPE_ERROR: The JSON document has an improper structure/);
      });
  });
});

describe('from Buffer', () => {
  const buffer = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'));
  const expected = JSON.parse(buffer.toString());

  it('parse()', () => {
    const document = JSONAsync.parse<FeatureCollection>(buffer);
    const geometry = document.get().features.get()[0].get().geometry.toObject() as Polygon;
    assert.deepEqual(geometry.coordinates[0], expected.features[0].geometry.coordinates[0]);
  });

  it('parseAsync()', (done) => {
    JSONAsync.parseAsync<FeatureCollection>(buffer)
      .then((document) => {
        assert.isObject(document.get());
        assert.sameMembers(Object.keys(document.get()), ['type', 'features']);
        const features = document.get().features.toObject();
        assert.deepEqual(features, expected.features);
        done();
      })
      .catch(done);
  });
});

describe('latency', () => {
  it('must have a configurable latency', () => {
    assert.isNumber(JSONAsync.latency);
    assert.strictEqual(JSONAsync.latency, 5);
    JSONAsync.latency = 10;
    assert.strictEqual(JSONAsync.latency, 10);
  });
});
