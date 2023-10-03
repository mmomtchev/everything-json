import { JSON as JSONAsync } from 'everything-json';

exports.mochaHooks = {
  afterEach() {
    if (!global.gc)
      throw new Error('expose-gc not enabled');
    global.gc();
  },
  beforeAll() {
    console.log(`Using simdjson ${JSONAsync.simdjson_version} in ${JSONAsync.simd} mode\n`);
  }
};
