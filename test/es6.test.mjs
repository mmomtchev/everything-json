import { assert } from 'chai';

import { JSON } from '../lib/index.mjs';

describe('ES6 import', () => {
  it('parse()', () => {
    assert.isFunction(JSON.parse);
  })
});
