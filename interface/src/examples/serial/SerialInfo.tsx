import React, { FC } from 'react';
import { Typography, List, ListItem, ListItemText } from '@mui/material';
import { SectionContent } from '../../components';

const SerialInfo: FC = () => (
  <SectionContent title='Serial' titleGutter>
    <Typography variant="body1" paragraph>
      This service monitors Serial2 (GPIO16/17 on ESP32) and streams the data
      across all communication channels in real-time.
    </Typography>
    
    <Typography variant="h6" gutterBottom>
      Features
    </Typography>
    <List>
      <ListItem>
        <ListItemText 
          primary="REST API" 
          secondary="Returns the last received line (polling)" 
        />
      </ListItem>
      <ListItem>
        <ListItemText 
          primary="WebSocket" 
          secondary="Real-time streaming of all lines as they arrive" 
        />
      </ListItem>
      <ListItem>
        <ListItemText 
          primary="MQTT" 
          secondary="Publishes each line to topic: weighsoft/serial/{id}/data" 
        />
      </ListItem>
      <ListItem>
        <ListItemText 
          primary="BLE" 
          secondary="Notifies subscribers when new lines arrive" 
        />
      </ListItem>
    </List>
    
    <Typography variant="h6" gutterBottom sx={{ mt: 2 }}>
      Hardware Setup
    </Typography>
    <Typography variant="body2">
      Connect your serial device to:
    </Typography>
    <List dense>
      <ListItem>
        <ListItemText 
          primary="RX (GPIO16)" 
          secondary="Receives data from external device TX" 
        />
      </ListItem>
      <ListItem>
        <ListItemText 
          primary="TX (GPIO17)" 
          secondary="Not currently used by this service" 
        />
      </ListItem>
      <ListItem>
        <ListItemText 
          primary="GND" 
          secondary="Common ground with external device" 
        />
      </ListItem>
    </List>
  </SectionContent>
);

export default SerialInfo;
