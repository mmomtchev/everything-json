exports.mochaHooks = {
  afterEach: (done: () => undefined) => {
    if (!global.gc)
      throw new Error('expose-gc not enabled');
    global.gc();
    done();
  }
};
