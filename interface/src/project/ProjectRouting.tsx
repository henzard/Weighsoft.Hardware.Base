import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';

import LedExample from '../examples/led/LedExample';
import Display from '../examples/display/Display';

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
      {
        // Display project routes
      }
      <Route path="display/*" element={<Display />} />
    </Routes>
  );
};

export default ProjectRouting;
