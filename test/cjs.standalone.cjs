const { assert } = require('chai');

const { JSON } = require('json-async');

describe('CJS require()', () => {
  it('parse()', () => {
    assert.isFunction(JSON.parse);
  });
});
