import { FC } from 'react';
import { Table, TableBody, TableCell, TableHead, TableRow } from '@mui/material';

import { SectionContent, FormLoader } from '../../components';
import { useRest } from '../../utils';
import { BleStatus } from '../../types/ble';
import * as BleApi from '../../api/ble';

const BleStatusForm: FC = () => {
  const { loadData, data, errorMessage } = useRest<BleStatus>({ read: BleApi.readBleStatus });

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    return (
      <Table size="small">
        <TableHead>
          <TableRow>
            <TableCell>Setting</TableCell>
            <TableCell>Value</TableCell>
          </TableRow>
        </TableHead>
        <TableBody>
          <TableRow>
            <TableCell>BLE Enabled</TableCell>
            <TableCell>{data.enabled ? 'Yes' : 'No'}</TableCell>
          </TableRow>
          <TableRow>
            <TableCell>Device Name</TableCell>
            <TableCell>{data.device_name || 'N/A'}</TableCell>
          </TableRow>
          <TableRow>
            <TableCell>MAC Address</TableCell>
            <TableCell>{data.mac_address || 'N/A'}</TableCell>
          </TableRow>
          <TableRow>
            <TableCell>Connected Devices</TableCell>
            <TableCell>{data.connected_devices}</TableCell>
          </TableRow>
        </TableBody>
      </Table>
    );
  };

  return (
    <SectionContent title="BLE Status" titleGutter>
      {content()}
    </SectionContent>
  );
};

export default BleStatusForm;
