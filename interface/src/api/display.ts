import { AxiosPromise } from 'axios';
import { AXIOS } from './endpoints';
import { DisplayData } from '../types/display';

export const DISPLAY_ENDPOINT = '/rest/display';

export function readDisplayData(): AxiosPromise<DisplayData> {
  return AXIOS.get(DISPLAY_ENDPOINT);
}

export function updateDisplayData(data: DisplayData): AxiosPromise<DisplayData> {
  return AXIOS.post(DISPLAY_ENDPOINT, data);
}
