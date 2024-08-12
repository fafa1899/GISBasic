import { defineConfig } from 'vite';
import cesium from 'vite-plugin-cesium';

export default defineConfig({
  server: {
    port: 3000
  },
  plugins: [cesium()]
});
