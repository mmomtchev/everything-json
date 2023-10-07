const path = require('path');
const binary = require('@mapbox/node-pre-gyp');

const binding_path = binary.find(path.resolve(path.join(__dirname, '..', 'package.json')));
const dll = require(binding_path);

const proxyHandler = {
  get(target, prop) {
    if (prop === 'toObject') return () => {
      if (target instanceof Array) {
        return target.map((e) => e instanceof dll.JSON ? e.toObject() : e);
      } else if (typeof target === 'object') {
        const r = {};
        for (const k of Object.keys(target)) 
          r[k] = target[k] instanceof dll.JSON ? target[k].toObject() : target[k];
        return r;
      } else {
        return target;
      }
    };
    const field = target[prop];
    if (field instanceof dll.JSON)
      return new Proxy(field.expand(), proxyHandler);
    return field;
  }
};

dll.JSON.prototype.proxify = function proxyify() {
  return new Proxy(this.expand(), proxyHandler);
};

module.exports = dll;
