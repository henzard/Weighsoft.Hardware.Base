import { AxiosPromise } from "axios";

import { AXIOS } from "../../api/endpoints";
import { LedExampleState } from "./types";

export function readLedState(): AxiosPromise<LedExampleState> {
  return AXIOS.get('/ledExample');
}

export function updateLedState(ledState: LedExampleState): AxiosPromise<LedExampleState> {
  return AXIOS.post('/ledExample', ledState);
}
