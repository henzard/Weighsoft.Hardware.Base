import { FC } from 'react';
import { ValidateFieldsError } from 'async-validator';

import { Button, Checkbox, TextField } from '@mui/material';
import SaveIcon from '@mui/icons-material/Save';

import {
  BlockFormControlLabel,
  ButtonRow,
  FormLoader,
  SectionContent
} from '../../components';
import { BleSettings } from '../../types/ble';
import { updateValue, useRest } from '../../utils';
import * as BleApi from '../../api/ble';

const BleSettingsForm: FC = () => {
  const {
    loadData, saving, data, setData, saveData, errorMessage
  } = useRest<BleSettings>({ read: BleApi.readBleSettings, update: BleApi.updateBleSettings });

  const updateFormValue = updateValue(setData);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const validateAndSubmit = async () => {
      try {
        await saveData();
      } catch (error: any) {
        if (error.errors) {
          const fieldErrors: ValidateFieldsError = error;
          console.error('Validation errors:', fieldErrors);
        }
      }
    };

    return (
      <>
        <BlockFormControlLabel
          control={
            <Checkbox
              name="enabled"
              checked={data.enabled}
              onChange={updateFormValue}
              disabled={saving}
            />
          }
          label="Enable BLE"
        />
        <TextField
          name="device_name"
          label="Device Name"
          fullWidth
          variant="outlined"
          value={data.device_name}
          onChange={updateFormValue}
          disabled={saving}
          margin="normal"
          helperText="The name that appears when scanning for BLE devices"
        />
        <ButtonRow mt={1}>
          <Button
            startIcon={<SaveIcon />}
            disabled={saving}
            variant="contained"
            color="primary"
            onClick={validateAndSubmit}
          >
            Save
          </Button>
        </ButtonRow>
      </>
    );
  };

  return (
    <SectionContent title="BLE Settings" titleGutter>
      {content()}
    </SectionContent>
  );
};

export default BleSettingsForm;
