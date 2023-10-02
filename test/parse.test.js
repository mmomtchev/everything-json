const fs = require('fs');
const path = require('path');
const { assert } = require('chai');

const JSONAsync = require('..').JSON;

describe('GeoJSON', () => {
  const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
  const expected = JSON.parse(text);

  it('get()', () => {
    const document = JSONAsync.parse(text);
    assert.isObject(document.get());
    assert.sameMembers(Object.keys(document.get()), ['type', 'features']);
    const features = document.get().features.get();
    assert.isArray(features);
    assert.instanceOf(features[0], JSONAsync);
    assert.closeTo(features[0].get()['geometry'].get()['coordinates'].get()[10].get()[2].get()[0].get(), -55.946, 1e-3);
  });

  it('toObject()', () => {
    const document = JSONAsync.parse(text);
    const coords = document.get().features.get()[0].get()['geometry'].get()['coordinates'].get()[10].toObject();
    assert.isArray(coords);
    assert.isArray(coords[4]);
    assert.isNumber(coords[4][1]);
    assert.deepEqual(document.get().features.get()[0].get().properties.toObject(), expected.features[0].properties);
  });

  it('parseAsync()', (done) => {
    JSONAsync.parseAsync(text)
      .then((document) => {
        assert.isObject(document.get());
        assert.sameMembers(Object.keys(document.get()), ['type', 'features']);
        const features = document.get().features.get();
        assert.isArray(features);
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
        assert.deepEqual(object.features[0].properties, expected.features[0].properties);
        done();
      })
      .catch(done);
  });
});
