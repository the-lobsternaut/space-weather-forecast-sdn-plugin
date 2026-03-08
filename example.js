/**
 * Example usage of Space Weather Forecast SDN Plugin
 */

import { SWFPlugin } from './dist/index.js';
import { readFileSync } from 'fs';

// Generate or load Ed25519 keypair
const privateKeyHex = readFileSync('./keypair.hex', 'utf-8').trim();

// Configure plugin
const plugin = new SWFPlugin({
  privateKey: privateKeyHex,
  gatewayUrl: 'https://gateway.space-data-network.org',
  gatewayToken: 'your-gateway-token',
  cacheDuration: 24,
  autoPublish: true
});

// Publish forecast
(async () => {
  try {
    await plugin.publishForecast();
    console.log('✓ Forecast published successfully');
  } catch (error) {
    console.error('✗ Error:', error);
    process.exit(1);
  }
})();
