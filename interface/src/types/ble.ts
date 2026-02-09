export interface BleSettings {
  enabled: boolean;
  device_name: string;
}

export interface BleStatus {
  enabled: boolean;
  connected_devices: number;
  device_name: string;
  mac_address: string;
}
