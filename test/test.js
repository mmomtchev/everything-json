const jsonAsync = require('../build/Release/json-async-native.node');
const fs = require('fs');

(async function () {
  let o;

  for (let i = 0; i < 10; i++) {
    o = await jsonAsync.parseAsync(fs.readFileSync(process.argv[2], 'utf8'));
  }

  console.log(Object.keys(o).slice(0, 10));
})();
