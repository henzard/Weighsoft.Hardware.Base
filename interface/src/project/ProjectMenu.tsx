import { FC } from 'react';

import { List } from '@mui/material';
import LightbulbIcon from '@mui/icons-material/Lightbulb';
import TvIcon from '@mui/icons-material/Tv';

import { PROJECT_PATH } from '../api/env';
import LayoutMenuItem from '../components/layout/LayoutMenuItem';

const ProjectMenu: FC = () => (
  <List>
    <LayoutMenuItem icon={LightbulbIcon} label="LED" to={`/${PROJECT_PATH}/led-example`} />
    <LayoutMenuItem icon={TvIcon} label="Display" to={`/${PROJECT_PATH}/display`} />
  </List>
);

export default ProjectMenu;
