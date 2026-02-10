import { AxiosPromise } from 'axios';
import { AXIOS } from './endpoints';
import { SerialData } from '../types/serial';

export const SERIAL_ENDPOINT = 'serial';

export function readSerialData(): AxiosPromise<SerialData> {
  return AXIOS.get(SERIAL_ENDPOINT);
}

export function updateSerialData(data: SerialData): AxiosPromise<SerialData> {
  return AXIOS.post(SERIAL_ENDPOINT, data);
}

