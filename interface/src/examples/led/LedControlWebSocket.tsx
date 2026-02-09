import { FC } from 'react';

import { Switch } from '@mui/material';

import { WEB_SOCKET_ROOT } from '../../api/endpoints';
import { BlockFormControlLabel, FormLoader, MessageBox, SectionContent } from '../../components';
import { updateValue, useWs } from '../../utils';

import { LedExampleState } from './types';

export const LED_EXAMPLE_WEBSOCKET_URL = WEB_SOCKET_ROOT + "ledExample";

const LedControlWebSocket: FC = () => {
  const { connected, updateData, data } = useWs<LedExampleState>(LED_EXAMPLE_WEBSOCKET_URL);

  const updateFormValue = updateValue(updateData);

  const content = () => {
    if (!connected || !data) {
      return (<FormLoader message="Connecting to WebSocket…" />);
    }
    return (
      <>
        <MessageBox
          level="info"
          message="Control the LED via WebSocket. Changes are instant and bidirectional - updates from other channels appear automatically."
          my={2}
        />
        <BlockFormControlLabel
          control={
            <Switch
              name="led_on"
              checked={data.led_on}
              onChange={updateFormValue}
              color="primary"
            />
          }
          label="LED On?"
        />
      </>
    );
  };

  return (
    <SectionContent title='WebSocket Control' titleGutter>
      {content()}
    </SectionContent>
  );
};

export default LedControlWebSocket;
