const CompressionPlugin = require('compression-webpack-plugin');

module.exports = {
  chainWebpack(config) {
    config.plugins.delete('prefetch');

    config.plugin('CompressionPlugin').use(CompressionPlugin);
  },
  configureWebpack: (config) => {
    config.devtool = 'source-map'
    config.mode = 'production'
    config.output.filename = 'js/[name].[hash:4].js';
    config.output.chunkFilename = 'js/[name].[hash:4].js';
  }
 }