#ifndef SerialState_h
#define SerialState_h

#include <StatefulService.h>

class SerialState {
 public:
  String lastLine;
  unsigned long timestamp;  // millis() when line was received
  
  static void read(SerialState& state, JsonObject& root) {
    root["last_line"] = state.lastLine;
    root["timestamp"] = state.timestamp;
  }
  
  static StateUpdateResult update(JsonObject& root, SerialState& state) {
    // Read-only service - no updates from REST/WS/etc
    return StateUpdateResult::UNCHANGED;
  }
};

#endif
