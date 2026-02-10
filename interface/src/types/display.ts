export interface DisplayData {
  line1: string;
  line2: string;
  i2cAddress: number;
  backlight: boolean;
  
  // Serial bridge fields
  bridgeMode: 'off' | 'websocket' | 'mqtt' | 'ble';
  serialDeviceIP: string;
  serialDevicePort: number;
  serialMqttTopic: string;
  serialBleServiceUuid: string;
  serialBleCharUuid: string;
}
