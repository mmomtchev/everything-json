const jsonAsync = require('../build/Debug/json-async-native.node');
const fs = require('fs');

console.time('read');
const text = fs.readFileSync(process.argv[2], 'utf8');
console.timeEnd('read');

console.time('Parse');
const doc = jsonAsync.JSON.parse(text);
console.timeEnd('Parse');

console.log(Object.keys(doc.get()).slice(0, 10));
console.log(doc.get()['a0f92b9f-6fac-46a9-9b93-9c73f095ae29'].get());
console.log(doc.get()['a0f92b9f-6fac-46a9-9b93-9c73f095ae29'].get()['description'].get());
