const { JSON } = require('.');
const _fs = require('fs');
const fs = _fs.promises;

fs.readFile('test/data/canada.json', 'utf-8')
  .then((data) => {
    console.time('parseAsync');
    return JSON.parseAsync(data);
  })
  .then((document) => {
    console.timeEnd('parseAsync');
    console.time('toObjectAsync');
    return document.toObjectAsync();
  })
  .then((object) => {
    console.timeEnd('toObjectAsync');
    console.log(object);
  });
