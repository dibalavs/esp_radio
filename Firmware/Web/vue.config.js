const CompressionPlugin = require('compression-webpack-plugin');

module.exports = {
  chainWebpack(config) {
    config.plugins.delete('prefetch');

    config.plugin('CompressionPlugin').use(CompressionPlugin);
  },
  configureWebpack: (config) => {
    config.devtool = 'source-map'
    config.output.filename = 'js/[name].[hash:2].js';
    config.output.chunkFilename = 'js/[name].[hash:2].js';
  }
 }