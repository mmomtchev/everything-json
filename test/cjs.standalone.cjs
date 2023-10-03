const { assert } = require('chai');

const { JSON } = require('everything-json');

describe('CJS require()', () => {
  it('parse()', () => {
    assert.isFunction(JSON.parse);
  });
});
