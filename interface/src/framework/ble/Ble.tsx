import { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';

import { Tab } from '@mui/material';

import { RouterTabs, useRouterTab, useLayoutTitle } from '../../components';
import BleSettingsForm from './BleSettingsForm';
import BleStatusForm from './BleStatusForm';

const Ble: FC = () => {
  useLayoutTitle("BLE");
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="status" label="BLE Status" />
        <Tab value="settings" label="BLE Settings" />
      </RouterTabs>
      <Routes>
        <Route path="status" element={<BleStatusForm />} />
        <Route path="settings" element={<BleSettingsForm />} />
        <Route path="/*" element={<Navigate replace to="status" />} />
      </Routes>
    </>
  );
};

export default Ble;
