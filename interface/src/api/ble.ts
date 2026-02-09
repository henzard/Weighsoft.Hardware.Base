import { AxiosPromise } from 'axios';
import { AXIOS } from './endpoints';
import { BleSettings, BleStatus } from '../types/ble';

export function readBleSettings(): AxiosPromise<BleSettings> {
  return AXIOS.get('/bleSettings');
}

export function updateBleSettings(bleSettings: BleSettings): AxiosPromise<BleSettings> {
  return AXIOS.post('/bleSettings', bleSettings);
}

export function readBleStatus(): AxiosPromise<BleStatus> {
  return AXIOS.get('/bleStatus');
}
