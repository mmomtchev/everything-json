import { assert } from 'chai';

import { JSON as JSONAsync } from 'everything-json';

const structured = {
  number: 42,
  array: [1, 2, 3],
  object: {
    number: 17,
    empty: {}
  },
  '0': '1',
  '1': true,
  two: null
};

const JSONtext = JSON.stringify(structured);

describe('TypeScript generics', () => {
  it('get()', () => {
    const document = JSONAsync.parse<typeof structured>(JSONtext);

    const object = document.get();
    assert.isObject(object);

    const array = object.array.get();
    assert.isArray<number[]>(array);
    assert.isNumber<number>(array[1]);

    const subObject = object.object.get();
    assert.isObject(subObject);
    assert.isNumber<number>(subObject.number);
    assert.isEmpty<Record<string, never>>(subObject.empty.get());

    const one = object[0];
    assert.isString<string>(one);

    const boolean = object['1'];
    assert.isBoolean<boolean>(boolean);

    const nothing = object.two;
    assert.isNull<null>(nothing);
  });

  it('toObject()', () => {
    const document = JSONAsync.parse<typeof structured>(JSONtext).toObject();

    const object = document;
    assert.isObject(object);

    const array = object.array;
    assert.isArray<number[]>(array);
    assert.isNumber<number>(array[1]);

    const subObject = object.object;
    assert.isObject(subObject);
    assert.isNumber<number>(subObject.number);
    assert.isEmpty<Record<string, never>>(subObject.empty);

    const one = object[0];
    assert.isString<string>(one);

    const boolean = object['1'];
    assert.isBoolean<boolean>(boolean);

    const nothing = object.two;
    assert.isNull<null>(nothing);
  });

  it('toObjectAsync()', (done) => {
    JSONAsync.parse<typeof structured>(JSONtext).toObjectAsync()
      .then((document) => {
        const object = document;
        assert.isObject(object);

        const array = object.array;
        assert.isArray<number[]>(array);
        assert.isNumber<number>(array[1]);

        const subObject = object.object;
        assert.isObject(subObject);
        assert.isNumber<number>(subObject.number);
        assert.isEmpty<Record<string, never>>(subObject.empty);

        const one = object[0];
        assert.isString<string>(one);

        const boolean = object['1'];
        assert.isBoolean<boolean>(boolean);

        const nothing = object.two;
        assert.isNull<null>(nothing);
        done();
      })
      .catch(done);
  });
});
