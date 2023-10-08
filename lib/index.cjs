const path = require('path');
const binary = require('@mapbox/node-pre-gyp');

const binding_path = binary.find(path.resolve(path.join(__dirname, '..', 'package.json')));
const dll = require(binding_path);

dll.JSON.symbolToObject = Symbol('toObject');
dll.JSON.symbolToObjectAsync = Symbol('toObjectAsync');
dll.JSON.symbolType = Symbol('type');

const proxyHandler = {
  get(target, prop) {
    if (typeof prop === 'symbol') {
      if (prop === dll.JSON.symbolToObject) return target.toObject.bind(target);
      if (prop === dll.JSON.symbolToObjectAsync) return target.toObjectAsync.bind(target);
      if (prop === dll.JSON.symbolType) return target.type;
      return target[prop];
    }
    const field = target.path(`/${prop}`);
    const type = field.type;
    if (type === 'array' || type === 'object')
      return field.proxify();
    return field.get();
  },
  ownKeys(target) {
    return Object.keys(target.get());
  },
  getOwnPropertyDescriptor(target, prop) {
    try {
      target.path(`/${prop}`);
      return {
        configurable: true,
        enumerable: true
      };
    } catch {
      return {};
    }
  }
};

dll.JSON.prototype.proxify = function proxyify() {
  return new Proxy(this, proxyHandler);
};

module.exports = dll;
