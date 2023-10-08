const path = require('path');
const binary = require('@mapbox/node-pre-gyp');

const binding_path = binary.find(path.resolve(path.join(__dirname, '..', 'package.json')));
const dll = require(binding_path);

const proxyHandler = {
  get(target, prop) {
    if (prop === 'toObject') return target.__JSON__.toObject.bind(target.__JSON__);
    if (prop === 'toObjectAsync') return target.__JSON__.toObjectAsync.bind(target.__JSON__);
    const field = target[prop];
    if (field instanceof dll.JSON)
      return field.proxify();
    return field;
  }
};

dll.JSON.prototype.proxify = function proxyify() {
  const p = new Proxy(this.expand(), proxyHandler);
  Object.defineProperty(p, '__JSON__', {
    value: this,
    enumerable: false,
    configurable: false,
    writable: false
  });
  return p;
};

module.exports = dll;
