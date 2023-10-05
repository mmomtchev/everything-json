const fs = require('fs');
const path = require('path');
const { assert } = require('chai');

const JSONAsync = require('..').JSON;

const bench = fs.readdirSync(__dirname).filter((file) => file.match(/\.bench\.js$/));

const files = ['canada.json', 'gsoc-2018.json', 'twitter.json'];
const testDataFn = [
  (data) => {
    assert.isArray(data);
    assert.closeTo(data[0], -65.614, 1e-3);
    assert.closeTo(data[1], 43.42, 1e-3);
  },
  (data) => {
    assert.isObject(data);
    assert.strictEqual(data.sponsor.url, 'https://apache.org');
  },
  (data) => {
    assert.isObject(data);
    assert.strictEqual(data.max_id, 505874924095815700);
  }
];
const testDataGet = [
  {
    object: (document) => document.features[0].geometry.coordinates[0][0],
    everything: (document) => document.get().features.get()[0].get().geometry.get().coordinates.get()[0].get()[0].toObject(),
    simdjson: (document) => document.valueForKeyPath('features[0].geometry.coordinates[0][0]'),
  },
  {
    object: (document) => document[1],
    everything: (document) => document.get()[1].toObject(),
    simdjson: (document) => document.valueForKeyPath('[1]')
  },
  {
    object: (document) => document.search_metadata,
    everything: (document) => document.get().search_metadata.toObject(),
    simdjson: (document) => document.valueForKeyPath('search_metadata')
  }
];

const testDataPath = path.resolve(__dirname, '..', 'test', 'data');
const testJSONUTF8 = files.map((file) => fs.readFileSync(path.join(testDataPath, file), 'utf-8'));
const testJSONBuffer = files.map((file) => fs.readFileSync(path.join(testDataPath, file)));

module.exports = {
  testJSONUTF8,
  testJSONBuffer
};

console.log(`Using simdjson ${JSONAsync.simdjson_version} in ${JSONAsync.simd} mode\n`);

(async () => {
  for (const b of bench) {
    if (process.argv[2] && !b.match(process.argv[2]))
      continue;
    for (const t in files) {
      console.log(`${files[t]} : ${b}`);
      // eslint-disable-next-line no-await-in-loop
      await require(`${__dirname}/${b}`)(testDataFn[t], testDataGet[t], testJSONUTF8[t], testJSONBuffer[t]);
      console.log('\n\n');
    }
  }
})();
