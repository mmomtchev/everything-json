import { JSON as JSONAsync } from 'everything-json';

exports.mochaHooks = {
  afterEach() {
    if (!global.gc)
      throw new Error('expose-gc not enabled');
    global.gc();
  },
  beforeAll() {
    console.log(`Using simdjson ${JSONAsync.simdjson_version} in ${JSONAsync.simd} mode\n`);
    
    // @ts-ignore
    if (JSONAsync.debug) console.log('debug build\n');

    if (process.env.MOCHA_REPEAT) {
      const r = +process.env.MOCHA_REPEAT;
      console.log(`Repeating each test ${r} times`);
      this.repeats(r);
    }
  }
};
