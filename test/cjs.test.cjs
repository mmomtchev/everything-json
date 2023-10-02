const { assert } = require('chai');

const { JSON } = require('../lib/index.cjs');

describe('CJS require()', () => {
  it('parse()', () => {
    assert.isFunction(JSON.parse);
  })
});
