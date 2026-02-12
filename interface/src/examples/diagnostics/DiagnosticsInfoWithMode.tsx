import React, { FC } from 'react';
import { Box } from '@mui/material';
import { UartModeSwitcher } from '../../components';
import DiagnosticsInfo from './DiagnosticsInfo';

const DiagnosticsInfoWithMode: FC = () => {
  return (
    <Box>
      <UartModeSwitcher currentMode="diagnostics" />
      <DiagnosticsInfo />
    </Box>
  );
};

export default DiagnosticsInfoWithMode;
