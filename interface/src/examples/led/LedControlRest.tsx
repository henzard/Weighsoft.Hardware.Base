import { FC } from 'react';

import { Button, Checkbox } from '@mui/material';
import SaveIcon from '@mui/icons-material/Save';

import { SectionContent, FormLoader, BlockFormControlLabel, ButtonRow, MessageBox } from '../../components';
import { updateValue, useRest } from '../../utils';

import * as LedApi from './api';
import { LedExampleState } from './types';

const LedControlRest: FC = () => {
  const {
    loadData, saveData, saving, setData, data, errorMessage
  } = useRest<LedExampleState>({ read: LedApi.readLedState, update: LedApi.updateLedState });

  const updateFormValue = updateValue(setData);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    return (
      <>
        <MessageBox
          level="info"
          message="Control the LED via REST API. Changes require clicking Save."
          my={2}
        />
        <BlockFormControlLabel
          control={
            <Checkbox
              name="led_on"
              disabled={saving}
              checked={data.led_on}
              onChange={updateFormValue}
              color="primary"
            />
          }
          label="LED On?"
        />
        <ButtonRow mt={1}>
          <Button startIcon={<SaveIcon />} disabled={saving} variant="contained" color="primary" type="submit" onClick={saveData}>
            Save
          </Button>
        </ButtonRow>
      </>
    );
  };

  return (
    <SectionContent title='REST Control' titleGutter>
      {content()}
    </SectionContent>
  );
};

export default LedControlRest;
