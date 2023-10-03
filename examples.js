const { JSON } = require('.');
const fs = require('fs');

const document = JSON.parse(fs.readFileSync('test/data/canada.json'));
console.log(document.get().features.get()[0].get().geometry.get()
  .coordinates.get()[10].toObject());

(async function() {
const document = await JSON.parseAsync(await fs.promises.readFile('test/data/canada.json'));
  console.log(await document.get().features.get()[0].get().geometry.get()
    .coordinates.get()[10].toObjectAsync());
})();

