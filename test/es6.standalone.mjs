import { assert } from 'chai';

import { JSON } from 'everything-json';

describe('ES6 import', () => {
  it('parse()', () => {
    assert.isFunction(JSON.parse);
  });
});
