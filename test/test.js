const fs = require('fs');
const path = require('path');
const { assert } = require('chai');

const { JSON } = require('..');

describe('GeoJSON', () => {
  it('get()', () => {
    const document = JSON.parse(fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8'));
    assert.isObject(document.get());
    assert.sameMembers(Object.keys(document.get()), ['type', 'features']);
    const features = document.get().features.get();
    assert.isArray(features);
    assert.instanceOf(features[0], JSON);
    assert.closeTo(features[0].get()['geometry'].get()['coordinates'].get()[10].get()[2].get()[0].get(), -55.946, 1e-3);
  });

  it('toObject()', () => {
    const document = JSON.parse(fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8'));
    const coords = document.get().features.get()[0].get()['geometry'].get()['coordinates'].get()[10].toObject();
    assert.isArray(coords);
    assert.isArray(coords[4]);
    assert.isNumber(coords[4][1]);
  });
});
