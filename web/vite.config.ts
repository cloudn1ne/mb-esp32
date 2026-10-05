import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  // relative base so the built app works when served from LittleFS ("/www/")
  base: './',
  server: {
    // dev-mode: reach the ESP32 REST API while on the MonkeyBoard AP
    proxy: {
      '/api': 'http://192.168.2.1:80',
    },
  },
  resolve: {
    alias: {
      '~bootstrap': './node_modules/bootstrap',
    }
  }
})