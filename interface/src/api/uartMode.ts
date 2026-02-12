import { AxiosPromise } from 'axios';
import { AXIOS } from '../api/endpoints';
import { UartModeData } from '../types/uartMode';

export function readUartMode(): AxiosPromise<UartModeData> {
  return AXIOS.get('/uartMode');
}

export function updateUartMode(data: Record<string, any>): AxiosPromise<UartModeData> {
  return AXIOS.post('/uartMode', data);
}
