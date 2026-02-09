import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';

import LedExample from '../examples/led/LedExample';
import SerialMonitor from '../examples/serial/SerialMonitor';

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
        // Serial Monitor project routes
      }
      <Route path="serial/*" element={<SerialMonitor />} />
    </Routes>
  );
};

export default ProjectRouting;
