import React, { FC, useEffect, useState } from 'react';
import { Box, Typography, Link, Tooltip } from '@mui/material';
import { readVersion } from '../api/version';
import { VersionInfo } from '../types/version';
import InfoIcon from '@mui/icons-material/Info';

const VersionDisplay: FC = () => {
  const [version, setVersion] = useState<VersionInfo | null>(null);

  useEffect(() => {
    readVersion()
      .then((response) => setVersion(response.data))
      .catch((error) => console.error('Failed to load version:', error));
  }, []);

  if (!version) {
    return null;
  }

  return (
    <Box
      sx={{
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        py: 1,
        px: 2,
        borderTop: 1,
        borderColor: 'divider',
        bgcolor: 'background.paper',
      }}
    >
      <Tooltip
        title={
          <Box>
            <Typography variant="caption" display="block">
              <strong>Version:</strong> {version.version}
            </Typography>
            <Typography variant="caption" display="block">
              <strong>API:</strong> {version.apiVersion}
            </Typography>
            <Typography variant="caption" display="block">
              <strong>Build:</strong> {version.buildDate} {version.buildTime}
            </Typography>
          </Box>
        }
        arrow
      >
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.5 }}>
          <InfoIcon fontSize="small" sx={{ color: 'text.secondary' }} />
          <Typography variant="caption" color="text.secondary">
            {version.projectName} v{version.version}
          </Typography>
        </Box>
      </Tooltip>
      {version.projectUrl && (
        <Typography variant="caption" color="text.secondary" sx={{ mx: 1 }}>
          •
        </Typography>
      )}
      {version.projectUrl && (
        <Link
          href={version.projectUrl}
          target="_blank"
          rel="noopener noreferrer"
          sx={{ textDecoration: 'none' }}
        >
          <Typography variant="caption" color="primary">
            GitHub
          </Typography>
        </Link>
      )}
    </Box>
  );
};

export default VersionDisplay;
