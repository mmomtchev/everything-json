const fs = require('fs');
const yj = require('yieldable-json');
const simdjson = require('simdjson');

console.time('read');
const data = fs.readFileSync(process.argv[2], 'utf8');
console.timeEnd('read');

console.time('JSON');
const obj = JSON.parse(data);
console.timeEnd('JSON');

try {
  console.time('SIMD');
  const simd = simdjson.parse(data);
  console.timeEnd('SIMD');
} catch {
  console.log('SIMD failed');
}

try {
  console.time('SIMD parse-only');
  const simd2 = simdjson.isValid(data);
  console.timeEnd('SIMD parse-only');
} catch {
  console.log('SIMD failed');
}

console.time('yieldable JSON');
const yobj = yj.parseAsync(data, (err, data) => {
  console.timeEnd('yieldable JSON', err);
});
