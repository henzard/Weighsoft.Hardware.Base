import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';

import LedExample from '../examples/led/LedExample';

const ProjectRouting: FC = () => {
  return (
    <Routes>
      {
        // Default route for LED example project
      }
      <Route path="/*" element={<Navigate to="led-example/information" />} />
      {
        // LED Example project routes
      }
      <Route path="led-example/*" element={<LedExample />} />
    </Routes>
  );
};

export default ProjectRouting;
