import { defineConfig } from 'vite';
import cesium from 'vite-plugin-cesium';

export default defineConfig({
  server: {
    port: 3000,
    proxy: {
      '/geoserver': {
        target: 'http://localhost:8080',
        changeOrigin: true
      }
    }
  },
  plugins: [cesium()]
});
