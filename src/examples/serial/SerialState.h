#ifndef SerialState_h
#define SerialState_h

#include <StatefulService.h>

#define SERIAL_DEFAULT_BAUDRATE 115200
#define SERIAL_MIN_BAUDRATE 300
#define SERIAL_MAX_BAUDRATE 2000000

class SerialState {
 public:
  // Data fields (read from serial, broadcast to channels)
  String lastLine;           // Full original line from serial
  String weight;             // Extracted weight value (empty if regex failed)
  unsigned long timestamp;   // millis() when line was received

  // Configuration fields (user-configurable via REST/UI)
  uint32_t baudrate;         // 9600, 19200, 38400, 57600, 115200, etc.
  uint8_t databits;          // 5, 6, 7, or 8
  uint8_t stopbits;          // 1 or 2
  uint8_t parity;            // 0=None, 1=Even, 2=Odd
  String regexPattern;       // Pattern to extract weight (e.g. first capture group)

  static void read(SerialState& state, JsonObject& root) {
    root["last_line"] = state.lastLine;
    root["weight"] = state.weight;
    root["timestamp"] = state.timestamp;
    root["baud_rate"] = state.baudrate;
    root["data_bits"] = state.databits;
    root["stop_bits"] = state.stopbits;
    root["parity"] = state.parity;
    root["regex_pattern"] = state.regexPattern;
  }

  static StateUpdateResult update(JsonObject& root, SerialState& state) {
    StateUpdateResult result = StateUpdateResult::UNCHANGED;

    if (root.containsKey("baud_rate")) {
      uint32_t v = root["baud_rate"];
      if (v >= SERIAL_MIN_BAUDRATE && v <= SERIAL_MAX_BAUDRATE && v != state.baudrate) {
        state.baudrate = v;
        result = StateUpdateResult::CHANGED;
      }
    }
    if (root.containsKey("data_bits")) {
      uint8_t v = root["data_bits"];
      if (v >= 5 && v <= 8 && v != state.databits) {
        state.databits = v;
        result = StateUpdateResult::CHANGED;
      }
    }
    if (root.containsKey("stop_bits")) {
      uint8_t v = root["stop_bits"];
      if ((v == 1 || v == 2) && v != state.stopbits) {
        state.stopbits = v;
        result = StateUpdateResult::CHANGED;
      }
    }
    if (root.containsKey("parity")) {
      uint8_t v = root["parity"];
      if (v <= 2 && v != state.parity) {
        state.parity = v;
        result = StateUpdateResult::CHANGED;
      }
    }
    if (root.containsKey("regex_pattern")) {
      String v = root["regex_pattern"].as<String>();
      if (v != state.regexPattern) {
        state.regexPattern = v;
        result = StateUpdateResult::CHANGED;
      }
    }
    return result;
  }
};

#endif
