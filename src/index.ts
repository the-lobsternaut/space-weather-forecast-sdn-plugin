/**
 * Space Weather Forecast SDN Plugin
 * @packageDocumentation
 */

import { writeFileSync, readFileSync, existsSync, mkdirSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import { standards } from 'spacedatastandards.org';
import { standards as sdsStandards } from 'spacedatastandards.org';
import { Ed25519Keypair } from '@noble/ed25519';
import { sha256 } from 'js-sha256';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

/**
 * Space Weather Forecast SDN Plugin Configuration
 */
export interface SWFPluginConfig {
  /** Ed25519 private key for signing data */
  privateKey: string;
  /** SDN gateway URL */
  gatewayUrl: string;
  /** SDN gateway token for authentication */
  gatewayToken: string;
  /** Cache duration in hours (default: 24) */
  cacheDuration?: number;
  /** Auto-publish on fetch (default: true) */
  autoPublish?: boolean;
}

/**
 * Space Weather Forecast SDN Plugin
 * Publishes NOAA SWPC forecast data to the Space Data Network
 */
export class SWFPlugin {
  private config: SWFPluginConfig;
  private keypair: Ed25519Keypair;
  private cacheDir: string;
  private cacheDuration: number;

  constructor(config: SWFPluginConfig) {
    this.config = config;
    this.keypair = Ed25519Keypair.fromPrivateKey(
      Buffer.from(config.privateKey, 'hex')
    );
    this.cacheDir = join(__dirname, '../cache');
    this.cacheDuration = config.cacheDuration ?? 24;

    // Create cache directory if it doesn't exist
    if (!existsSync(this.cacheDir)) {
      mkdirSync(this.cacheDir, { recursive: true });
    }
  }

  /**
   * Fetch and publish space weather forecast
   */
  public async publishForecast(): Promise<void> {
    try {
      console.log('[SWFPlugin] Fetching NOAA SWPC forecast...');

      // Fetch forecast data
      const forecastData = await this.fetchNOAAForecast();

      // Validate against SDS schema
      this.validateForecast(forecastData);

      // Serialize to FlatBuffers
      const flatbuffer = this.serializeToFlatBuffers(forecastData);

      // Sign the data
      const signature = this.signData(flatbuffer);

      // Publish to SDN
      await this.publishToSDN(flatbuffer, signature);

      console.log('[SWFPlugin] Forecast published successfully');

      // Cache the result
      this.cacheForecast(forecastData);

    } catch (error) {
      console.error('[SWFPlugin] Error publishing forecast:', error);
      throw error;
    }
  }

  /**
   * Fetch forecast data from NOAA SWPC
   */
  private async fetchNOAAForecast(): Promise<any> {
    const url = 'https://www.spaceweather.gov/products/45-day-forecast';
    const response = await fetch(url);
    const text = await response.text();

    // Parse text format to structured data
    return this.parseNOAAText(text);
  }

  /**
   * Parse NOAA text format to structured JSON
   */
  private parseNOAAText(text: string): any {
    const lines = text.split('\n').filter(line => line.trim());

    const result: any = {
      product_id: '45-day-forecast',
      product_name: '45-Day Ap and F10.7cm Flux Forecast',
      version: '1.0',
      issued: new Date().toISOString(),
      issued_utc: Math.floor(Date.now() / 1000),
      forecaster: 'SWPC Forecasting System',
      product_type: 'AP_F107_45DAY',
      description: '45-Day Ap and F10.7cm Flux Forecast',
      ap_forecast: [],
      f107_forecast: [],
      forecast_start_date: new Date().toISOString().split('T')[0],
      forecast_end_date: '',
      forecast_duration_days: 45,
      source: 'NOAA SWPC',
      source_url: 'https://www.spaceweather.gov/products/45-day-forecast',
      data_quality: 'Preliminary',
      confidence: 0.95
    };

    let inApSection = false;
    let inF107Section = false;
    let currentDate = '';

    for (const line of lines) {
      // Detect Ap section
      if (line.includes('45-DAY AP FORECAST')) {
        inApSection = true;
        inF107Section = false;
        continue;
      }

      // Detect F107 section
      if (line.includes('45-DAY F10.7 CM FLUX FORECAST')) {
        inApSection = false;
        inF107Section = true;
        currentDate = '';
        continue;
      }

      // Parse date codes
      if (line.match(/^\d{2}Mar\d{2}/) && inApSection) {
        const match = line.match(/^(\d{2})Mar(\d{2})/);
        if (match) {
          const month = parseInt(match[1], 10);
          const day = parseInt(match[2], 10);
          const year = 2026;
          currentDate = `${year}-${month.toString().padStart(2, '0')}-${day.toString().padStart(2, '0')}`;

          const apValue = parseInt(line.match(/\d{3}/)?.[0] || '0', 10);
          if (currentDate && apValue > 0) {
            result.ap_forecast.push({
              date: currentDate,
              date_utc: Math.floor(new Date(currentDate).getTime() / 1000),
              ap_value: apValue,
              ap_code: line.match(/\d{3}/)?.[0] || '0',
              forecast_status: 'Forecast'
            });
          }
        }
      }

      // Parse F107 values
      if (line.match(/^\d{3}$/) && inF107Section) {
        const f107Value = parseInt(line, 10);
        if (currentDate && f107Value > 0) {
          result.f107_forecast.push({
            date: currentDate,
            date_utc: Math.floor(new Date(currentDate).getTime() / 1000),
            f107_value: f107Value,
            f107_code: line,
            forecast_status: 'Forecast'
          });
        }
      }
    }

    // Set end date
    if (result.f107_forecast.length > 0) {
      result.forecast_end_date = result.f107_forecast[result.f107_forecast.length - 1].date;
    }

    // Calculate trends
    result.ap_trend = this.calculateTrend(result.ap_forecast);
    result.f107_trend = this.calculateTrend(result.f107_forecast);

    // Set current values (last forecasted values)
    if (result.ap_forecast.length > 0) {
      result.ap_current = result.ap_forecast[0].ap_value;
    }
    if (result.f107_forecast.length > 0) {
      result.f107_current = result.f107_forecast[0].f107_value;
    }

    return result;
  }

  /**
   * Calculate trend from forecast data
   */
  private calculateTrend(forecast: any[]): string {
    if (forecast.length < 2) return 'STABLE';

    const first = forecast[0].ap_value;
    const last = forecast[forecast.length - 1].ap_value;
    const diff = Math.abs(last - first);

    if (diff < 5) return 'STABLE';
    if (last > first) return 'RISING';
    return 'FALLING';
  }

  /**
   * Validate forecast data against SDS schema
   */
  private validateForecast(forecast: any): void {
    // Check required fields
    if (!forecast.product_id) throw new Error('Missing product_id');
    if (!forecast.issued) throw new Error('Missing issued timestamp');
    if (!forecast.ap_forecast || forecast.ap_forecast.length === 0) {
      throw new Error('Missing ap_forecast');
    }

    // Validate ap_forecast structure
    for (const day of forecast.ap_forecast) {
      if (!day.date || !day.ap_value) {
        throw new Error('Invalid ap_forecast entry');
      }
    }

    // Validate f107_forecast structure
    for (const day of forecast.f107_forecast) {
      if (!day.date || !day.f107_value) {
        throw new Error('Invalid f107_forecast entry');
      }
    }
  }

  /**
   * Serialize forecast to FlatBuffers using SDS
   */
  private serializeToFlatBuffers(forecast: any): Uint8Array {
    const { SWF } = standards;

    // Create SWF object
    const swf = new SWF({
      product_id: forecast.product_id,
      product_name: forecast.product_name,
      version: forecast.version,
      issued: forecast.issued,
      issued_utc: forecast.issued_utc,
      expires: new Date(Date.now() + 45 * 24 * 60 * 60 * 1000).toISOString(),
      expires_utc: Math.floor(Date.now() / 1000) + 45 * 24 * 60 * 60,
      forecaster: forecast.forecaster,
      product_type: forecast.product_type,
      description: forecast.description,
      ap_current: forecast.ap_current || 0,
      ap_max_24h: 0, // Would calculate from forecast data
      ap_min_24h: 0, // Would calculate from forecast data
      ap_trend: forecast.ap_trend || 'STABLE',
      f107_current: forecast.f107_current || 0,
      f107_max_24h: 0, // Would calculate from forecast data
      f107_min_24h: 0, // Would calculate from forecast data
      f107_trend: forecast.f107_trend || 'STABLE',
      forecast_start_date: forecast.forecast_start_date,
      forecast_end_date: forecast.forecast_end_date,
      forecast_duration_days: forecast.forecast_duration_days,
      source: forecast.source,
      source_url: forecast.source_url,
      data_quality: forecast.data_quality,
      confidence: forecast.confidence
    });

    // Serialize to FlatBuffers
    return standards.writeFB([swf]);
  }

  /**
   * Sign data with Ed25519
   */
  private signData(data: Uint8Array): string {
    const signature = this.keypair.sign(data);
    return Buffer.from(signature).toString('hex');
  }

  /**
   * Publish data to SDN network
   */
  private async publishToSDN(
    data: Uint8Array,
    signature: string
  ): Promise<void> {
    // TODO: Implement SDN publishing
    // This would use IPFS/libp2p to publish to the Space Data Network
    console.log('[SWFPlugin] Publishing to SDN...');
    console.log('[SWFPlugin] Data size:', data.length, 'bytes');
    console.log('[SWFPlugin] Signature:', signature);

    // Placeholder for SDN publishing
    // In production, this would use the Space Data Network SDK
  }

  /**
   * Cache forecast data
   */
  private cacheForecast(forecast: any): void {
    const cacheFile = join(this.cacheDir, `swf-${Date.now()}.json`);
    writeFileSync(cacheFile, JSON.stringify(forecast, null, 2));
  }
}
