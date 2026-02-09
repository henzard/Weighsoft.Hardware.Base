import { FC } from 'react';

import { List } from '@mui/material';
import LightbulbIcon from '@mui/icons-material/Lightbulb';
import SerialPortIcon from '@mui/icons-material/Cable';

import { PROJECT_PATH } from '../api/env';
import LayoutMenuItem from '../components/layout/LayoutMenuItem';

const ProjectMenu: FC = () => (
  <List>
    <LayoutMenuItem icon={LightbulbIcon} label="LED Example" to={`/${PROJECT_PATH}/led-example`} />
    <LayoutMenuItem icon={SerialPortIcon} label="Serial Monitor" to={`/${PROJECT_PATH}/serial`} />
  </List>
);

export default ProjectMenu;
