/**
 * SWPC Forecast WASM Parser - Node.js wrapper
 * Follows spacedatastandards.org/wasm/node/index.mjs pattern
 */

import { createRequire } from 'module';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

let _module = null;

export async function init() {
    if (_module) return _module;

    const factory = (await import('./swpc_forecast.js')).default;
    _module = await factory();
    return _module;
}

export { _module as module };
