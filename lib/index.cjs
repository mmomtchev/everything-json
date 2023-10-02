const path = require('path');
const os = require('os');
const binary = require('@mapbox/node-pre-gyp');

const binding_path = binary.find(path.resolve(path.join(__dirname, '..', 'package.json')));
const dll = require(binding_path);

module.exports = dll;
