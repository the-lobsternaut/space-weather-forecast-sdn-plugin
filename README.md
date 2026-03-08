# Space Weather Forecast SDN Plugin

SDN (Space Data Network) plugin for publishing NOAA SWPC 45-Day Ap and F10.7cm Flux Forecast data.

## Overview

This plugin fetches, processes, and publishes space weather forecast data to the Space Data Network.

## Features

- Fetch NOAA SWPC 45-day forecast data
- Convert to FlatBuffers format using SDS schema
- Sign data with Ed25519 for cryptographic verification
- Publish to SDN network (IPFS/libp2p)
- Support for both Ap and F10.7 flux forecasts

## Installation

```bash
npm install spacedatastandards.org
```

## Usage

```javascript
import { SWFPlugin } from './sdn-plugin';

const plugin = new SWFPlugin({
  privateKey: 'your-ed25519-private-key',
  gatewayUrl: 'https://gateway.space-data-network.org',
  gatewayToken: 'your-gateway-token'
});

// Fetch and publish forecast
await plugin.publishForecast();
```

## Configuration

```javascript
{
  privateKey: string,           // Ed25519 private key for signing
  gatewayUrl: string,           // SDN gateway URL
  gatewayToken: string,         // Authentication token
  cacheDuration: number,        // Cache duration in hours (default: 24)
  autoPublish: boolean          // Auto-publish on fetch (default: true)
}
```

## Data Flow

1. Fetch forecast from NOAA SWPC (daily at 0000 UTC)
2. Parse text format to JSON structure
3. Validate against SDS schema
4. Serialize to FlatBuffers binary
5. Sign with Ed25519 private key
6. Publish to SDN network
7. Store IPFS CID for retrieval

## Schema Mapping

- **Product ID**: `product_id`
- **Issued Date**: `issued` / `issued_utc`
- **Ap Forecast**: `ap_forecast` array
- **F107 Forecast**: `f107_forecast` array
- **Current Values**: `ap_current`, `f107_current`
- **Trends**: `ap_trend`, `f107_trend`

## SDN Integration

This plugin integrates with the Space Data Network as a:
- **Publisher**: Pushes forecast data to subscribed nodes
- **Subscriber**: Can receive forecast data from other publishers
- **Relay**: Forwards forecasts to regional SDN nodes

## License

Apache 2.0
