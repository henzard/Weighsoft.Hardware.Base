
import React, { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';

import { Tab } from '@mui/material';

import { RouterTabs, useRouterTab, useLayoutTitle } from '../../components';

import DisplayInfo from './DisplayInfo';
import DisplayControl from './DisplayControl';
import DisplayBle from './DisplayBle';

const Display: FC = () => {
  useLayoutTitle("Display");
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="information" label="Information" />
        <Tab value="control" label="Control" />
        <Tab value="ble" label="BLE Control" />
      </RouterTabs>
      <Routes>
        <Route path="information" element={<DisplayInfo />} />
        <Route path="control" element={<DisplayControl />} />
        <Route path="ble" element={<DisplayBle />} />
        <Route path="/*" element={<Navigate replace to="information" />} />
      </Routes>
    </>
  );
};

export default Display;
