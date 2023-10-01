exports.mochaHooks = {
  afterEach: function (done) {
    gc();
    done();
  }
};
