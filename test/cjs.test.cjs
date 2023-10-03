const { assert } = require('chai');

const { JSON } = require('..');

describe('CJS require()', () => {
  it('parse()', () => {
    assert.isFunction(JSON.parse);
  });
});
