
import React, { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';

import { Tab } from '@mui/material';

import { RouterTabs, useRouterTab, useLayoutTitle } from '../../components';

import LedExampleInfo from './LedExampleInfo';
import LedControlRest from './LedControlRest';
import LedControlWebSocket from './LedControlWebSocket';

const LedExample: FC = () => {
  useLayoutTitle("LED Example");
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="information" label="Information" />
        <Tab value="rest" label="REST Control" />
        <Tab value="socket" label="WebSocket Control" />
      </RouterTabs>
      <Routes>
        <Route path="information" element={<LedExampleInfo />} />
        <Route path="rest" element={<LedControlRest />} />
        <Route path="socket" element={<LedControlWebSocket />} />
        <Route path="/*" element={<Navigate replace to="information" />} />
      </Routes>
    </>
  );
};

export default LedExample;
