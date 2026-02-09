import { AxiosPromise } from 'axios';
import { AXIOS } from './endpoints';
import { SerialData } from '../types/serial';

export const SERIAL_ENDPOINT = '/rest/serial';

export function readSerialData(): AxiosPromise<SerialData> {
  return AXIOS.get(SERIAL_ENDPOINT);
}
