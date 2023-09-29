const { JSON } = require('.');

const json = '{ "array": [ 1 , 2] }';
Promise.resolve(json)
  .then((data) => JSON.parseAsync(data))
  .then((document) => document.toObjectAsync())
  .then((object) => {
    console.log(object);
  });
