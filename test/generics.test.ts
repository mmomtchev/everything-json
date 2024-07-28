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
    assert.isArray<JSONAsync[]>(array);
    assert.isNumber<number>(array[1].get());

    const subObject = object.object.get();
    assert.isObject(subObject);
    assert.instanceOf<JSONAsync<number>>(subObject.number, JSONAsync);
    assert.isNumber<number>(subObject.number.get());
    assert.isEmpty<Record<string, never>>(subObject.empty.get());

    const one = object[0];
    assert.isString<string>(one.get());

    const boolean = object['1'];
    assert.isBoolean<boolean>(boolean.get());

    const nothing = object.two;
    assert.strictEqual<null>(nothing.get(), null);
  });

  it('expand()', () => {
    const document = JSONAsync.parse<typeof structured>(JSONtext);

    const object = document.expand();
    assert.isObject(object);

    const array = object.array.expand();
    assert.isArray<number[]>(array);
    assert.isNumber<number>(array[1]);

    const subObject = object.object.expand();
    assert.isObject(subObject);
    assert.isNumber<number>(subObject.number);
    assert.isEmpty<Record<string, never>>(subObject.empty.get());

    const one = object[0];
    assert.isString<string>(one);

    const boolean = object['1'];
    assert.isBoolean<boolean>(boolean);

    const nothing = object.two;
    assert.strictEqual<null>(nothing, null);
  });

  it('path()', () => {
    const document = JSONAsync.parse<typeof structured>(JSONtext);

    assert.isNumber<number>(document.path('/number').toObject());
    assert.isArray<number[]>(document.path('/array').toObject());
    assert.isNumber<number>(document.path('/array/1').toObject());
    assert.isNumber<number>(document.path('/array').path('/1').toObject());
    assert.isObject<typeof structured['object']>(document.path('/object').toObject());
    assert.isNumber<number>(document.path('/object/number').toObject());
    assert.isNumber<number>(document.path('/object').path('/number').toObject());
    assert.isObject<Record<string, never>>(document.path('/object/empty').toObject());
    assert.isObject<Record<string, never>>(document.path('/object').path('/empty').toObject());
    assert.isString<string>(document.path('/0').toObject());
    assert.isBoolean<boolean>(document.path('/1').toObject());
    assert.strictEqual<null>(document.path('/two').toObject(), null);
  });

  it('type()', () => {
    const document = JSONAsync.parse<typeof structured>(JSONtext);

    assert.strictEqual<'number'>(document.path('/number').type, 'number');
    assert.strictEqual<'array'>(document.path('/array').type, 'array');
    assert.strictEqual<'number'>(document.path('/array/1').type, 'number');
    assert.strictEqual<'number'>(document.path('/array').path('/1').type, 'number');
    assert.strictEqual<'object'>(document.path('/object').type, 'object');
    assert.strictEqual<'number'>(document.path('/object/number').type, 'number');
    assert.strictEqual<'number'>(document.path('/object').path('/number').type, 'number');
    assert.strictEqual<'object'>(document.path('/object/empty').type, 'object');
    assert.strictEqual<'object'>(document.path('/object').path('/empty').type, 'object');
    assert.strictEqual<'string'>(document.path('/0').type, 'string');
    assert.strictEqual<'boolean'>(document.path('/1').type, 'boolean');
    assert.strictEqual<'null'>(document.path('/two').type, 'null');
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
    assert.strictEqual<null>(nothing, null);
  });

  it('toObjectAsync()', (done) => {
    JSONAsync.parseAsync<typeof structured>(JSONtext)
      .then((json) => json.toObjectAsync())
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
        assert.strictEqual<null>(nothing, null);
        done();
      })
      .catch(done);
  });
});
