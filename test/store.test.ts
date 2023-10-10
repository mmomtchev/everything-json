import * as fs from 'fs';
import * as path from 'path';
import { assert } from 'chai';
import type { FeatureCollection } from 'geojson';

import { JSON as JSONAsync } from 'everything-json';

describe('uniqueness of objects', () => {
  it('get()', () => {
    const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
    const document = JSONAsync.parse<FeatureCollection>(text);

    const root1 = document.get();
    const root2 = document.get();
    assert.strictEqual(root1, root2);

    const feature1 = document.get().features.get()[0];
    const feature2 = document.get().features.get()[0];
    assert.strictEqual(feature1, feature2);
  });

  it('expand()', () => {
    const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
    const document = JSONAsync.parse<FeatureCollection>(text);

    const root1 = document.expand();
    const root2 = document.expand();
    assert.strictEqual(root1, root2);

    const feature1 = document.get().features.expand()[0];
    const feature2 = document.get().features.expand()[0];
    assert.strictEqual(feature1, feature2);
  });

  it('path()', () => {
    const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
    const document = JSONAsync.parse<FeatureCollection>(text);

    const feature1 = document.path('/features/0');
    const feature2 = document.path('/features/0');
    assert.strictEqual(feature1, feature2);
  });

  it('cross-method JSON objects', () => {
    const text = fs.readFileSync(path.resolve(__dirname, 'data', 'canada.json'), 'utf8');
    const document = JSONAsync.parse<FeatureCollection>(text);

    const feature1 = document.get().features.get()[0];
    const feature2 = document.path('/features/0');
    assert.strictEqual(feature1, feature2);
  });
});
