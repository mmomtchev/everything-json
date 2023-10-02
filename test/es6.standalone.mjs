import { assert } from 'chai';

import { JSON } from 'json-async';

describe('ES6 import', () => {
  it('parse()', () => {
    assert.isFunction(JSON.parse);
  });
});
