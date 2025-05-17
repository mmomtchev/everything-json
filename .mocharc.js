const process = require('node:process');

module.exports = {
  'spec': 'test/*.test.*s',
  'require': [
    'ts-node/register',
    'tsconfig-paths/register',
    './test/mocha.init.ts'
  ],
  'timeout': 4000,
  'node-option': process.versions.node.split('.')[0] >= 23 ?
    ['no-experimental-strip-types', 'expose-gc'] :
    ['expose-gc']
};
