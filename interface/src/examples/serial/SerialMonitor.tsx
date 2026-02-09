import React, { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';
import { Tab } from '@mui/material';
import { RouterTabs, useRouterTab, useLayoutTitle } from '../../components';

import SerialInfo from './SerialInfo';
import SerialRest from './SerialRest';
import SerialWebSocket from './SerialWebSocket';
import SerialBle from './SerialBle';

const SerialMonitor: FC = () => {
  useLayoutTitle("Serial");
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="information" label="Information" />
        <Tab value="rest" label="REST View" />
        <Tab value="stream" label="Live Stream" />
        <Tab value="ble" label="BLE Stream" />
      </RouterTabs>
      <Routes>
        <Route path="information" element={<SerialInfo />} />
        <Route path="rest" element={<SerialRest />} />
        <Route path="stream" element={<SerialWebSocket />} />
        <Route path="ble" element={<SerialBle />} />
        <Route path="/*" element={<Navigate replace to="information" />} />
      </Routes>
    </>
  );
};

export default SerialMonitor;
