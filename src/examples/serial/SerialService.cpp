#include <examples/serial/SerialService.h>

#ifdef ESP32
#include <HardwareSerial.h>
#endif

SerialService::SerialService(AsyncWebServer* server,
                             FS* fs,
                             SecurityManager* securityManager,
                             AsyncMqttClient* mqttClient
#if FT_ENABLED(FT_BLE)
                             ,BLEServer* bleServer
#endif
                             ) :
    _httpEndpoint(SerialState::read,
                  SerialState::update,
                  this,
                  server,
                  SERIAL_ENDPOINT_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _fsPersistence(SerialState::readConfig,
                   SerialState::updateConfig,
                   this,
                   fs,
                   SERIAL_CONFIG_FILE),
    _mqttPubSub(SerialState::read, SerialState::update, this, mqttClient),
    _webSocket(SerialState::read,
               SerialState::update,
               this,
               server,
               SERIAL_SOCKET_PATH,
               securityManager,
               AuthenticationPredicates::IS_AUTHENTICATED),
    _mqttClient(mqttClient)
#if FT_ENABLED(FT_BLE)
    ,_blePubSub(SerialState::read, SerialState::update, this, bleServer),
    _bleServer(bleServer),
    _bleService(nullptr),
    _bleCharacteristic(nullptr)
#endif
{
  _mqttBasePath = SettingValue::format("weighsoft/serial/#{unique_id}");
  _mqttName = SettingValue::format("serial-monitor-#{unique_id}");
  _mqttUniqueId = SettingValue::format("serial-#{unique_id}");
  _mqttClient->onConnect(std::bind(&SerialService::configureMqtt, this));
  _serialStarted = false;
  _suspended = false;  // Not suspended initially

  addUpdateHandler([this](const String& originId) {
    // Skip "serial_hw" (data from scale) and "init" (begin() will call applySerialConfig() itself)
    if (originId != "serial_hw" && originId != "init") {
      onConfigUpdated();
    }
  }, false);
}

void SerialService::begin() {
  // Load persisted config from flash (baud_rate, data_bits, stop_bits, parity, regex_pattern)
  _fsPersistence.readFromFS();
  Serial.printf("[Serial] Loaded config: %lu baud, %u%c%u, regex='%s'\n",
                (unsigned long)_state.baudrate, _state.databits,
                _state.parity == 0 ? 'N' : (_state.parity == 1 ? 'E' : 'O'),
                _state.stopbits, _state.regexPattern.c_str());

  // Clear runtime data (not persisted)
  _state.lastLine = "";
  _state.weight = "";
  _state.timestamp = 0;
  _lineBuffer = "";
  _serialStarted = false;

  // Start Serial2 with loaded config
  Serial.println(F("[Serial] Initializing Serial2..."));
  applySerialConfig();
}

void SerialService::loop() {
  // Skip reading if suspended (DiagnosticsService is using Serial2)
  if (_suspended) {
    return;
  }

  // Raw byte diagnostic: log every single byte that arrives on Serial2
  static unsigned long totalBytes = 0;
  while (Serial2.available()) {
    char c = Serial2.read();
    totalBytes++;
    // Log each byte as hex + printable char
    if (c >= 32 && c <= 126) {
      Serial.printf("[Serial] RX byte #%lu: 0x%02X '%c'\n", totalBytes, (uint8_t)c, c);
    } else {
      Serial.printf("[Serial] RX byte #%lu: 0x%02X (control)\n", totalBytes, (uint8_t)c);
    }

    // Line assembly (same logic as before)
    if (c == '\n' || c == '\r') {
      if (_lineBuffer.length() > 0) {
        Serial.printf("[Serial] Complete line: '%s'\n", _lineBuffer.c_str());
        String extracted = extractWeight(_lineBuffer);
        update([&](SerialState& state) {
          state.lastLine = _lineBuffer;
          state.weight = extracted;
          state.timestamp = millis();
          return StateUpdateResult::CHANGED;
        }, "serial_hw");
        if (extracted.length() > 0) {
          Serial.printf("[Serial] Weight extracted: '%s'\n", extracted.c_str());
        }
      }
      _lineBuffer = "";
    } else {
      _lineBuffer += c;
      if (_lineBuffer.length() > 512) {
        Serial.println(F("[Serial] WARNING: Line exceeded 512 chars, discarded"));
        _lineBuffer = "";
      }
    }
  }

  // Heartbeat every 5 seconds (shortened for debugging)
  static unsigned long lastDiag = 0;
  if (millis() - lastDiag >= 5000) {
    lastDiag = millis();
    Serial.printf("[Serial] Heartbeat: started=%d, totalRX=%lu, buffer=%d chars, baud=%lu\n",
                  _serialStarted, totalBytes, _lineBuffer.length(), (unsigned long)_state.baudrate);
  }
}

void SerialService::onConfigUpdated() {
  applySerialConfig();
}

uint32_t SerialService::getSerialConfig() {
#ifdef ESP32
  uint8_t d = _state.databits;
  uint8_t p = _state.parity;
  uint8_t s = _state.stopbits;
  if (d < 7) d = 7;
  if (d > 8) d = 8;
  if (s < 1) s = 1;
  if (s > 2) s = 2;
  if (p > 2) p = 0;
  if (d == 7) {
    if (p == 0) return s == 1 ? SERIAL_7N1 : SERIAL_7N2;
    if (p == 1) return s == 1 ? SERIAL_7E1 : SERIAL_7E2;
    return s == 1 ? SERIAL_7O1 : SERIAL_7O2;
  }
  if (p == 0) return s == 1 ? SERIAL_8N1 : SERIAL_8N2;
  if (p == 1) return s == 1 ? SERIAL_8E1 : SERIAL_8E2;
  return s == 1 ? SERIAL_8O1 : SERIAL_8O2;
#else
  return SERIAL_8N1;
#endif
}

void SerialService::applySerialConfig() {
#ifdef ESP32
  if (_serialStarted) {
    Serial2.end();
    Serial.println(F("[Serial] Stopping Serial2 for reconfiguration..."));
  }
  uint32_t baud = _state.baudrate;
  if (baud < SERIAL_MIN_BAUDRATE || baud > SERIAL_MAX_BAUDRATE) {
    baud = SERIAL_DEFAULT_BAUDRATE;
  }
  uint32_t config = getSerialConfig();
  Serial2.begin(baud, config, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  _serialStarted = true;
  Serial.printf("[Serial] Serial2 started: %lu baud, %u%c%u, RX=GPIO%d, TX=GPIO%d\n",
                (unsigned long)baud, _state.databits,
                _state.parity == 0 ? 'N' : (_state.parity == 1 ? 'E' : 'O'),
                _state.stopbits, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
#endif
}

String SerialService::extractWeight(const String& line) {
  const String& pattern = _state.regexPattern;
  if (pattern.length() == 0) {
    return "";
  }
  int openParen = pattern.indexOf('(');
  int closeParen = pattern.indexOf(')', openParen);
  if (openParen < 0 || closeParen <= openParen) {
    return "";
  }
  String prefix = pattern.substring(0, openParen);
  int searchStart = 0;
  if (prefix.length() > 0) {
    int prefixPos = line.indexOf(prefix);
    if (prefixPos < 0) return "";
    searchStart = prefixPos + prefix.length();
  }
  int i = searchStart;
  while (i < (int)line.length() && (line.charAt(i) == ' ' || line.charAt(i) == '\t')) {
    i++;
  }
  if (i >= (int)line.length() || !isDigit(line.charAt(i))) {
    return "";
  }
  int start = i;
  while (i < (int)line.length() && isDigit(line.charAt(i))) {
    i++;
  }
  if (i < (int)line.length() && line.charAt(i) == '.') {
    i++;
    while (i < (int)line.length() && isDigit(line.charAt(i))) {
      i++;
    }
  }
  return line.substring(start, i);
}

void SerialService::readSerial() {
  // Kept for API compatibility but logic is now in loop() for diagnostics
}

void SerialService::configureMqtt() {
  if (!_mqttClient->connected()) {
    return;
  }
  String pubTopic = _mqttBasePath + "/data";
  String subTopic = "";
  _mqttPubSub.configureTopics(pubTopic, subTopic);
  Serial.printf("[Serial] MQTT configured - topic: %s\n", pubTopic.c_str());
}

#if FT_ENABLED(FT_BLE)
void SerialService::configureBle() {
  if (_bleServer == nullptr) {
    Serial.println("[Serial] BLE server not available, skipping BLE configuration");
    return;
  }
  Serial.println("[Serial] Configuring BLE service...");
  _bleService = _bleServer->createService(BLE_SERVICE_UUID);
  _bleCharacteristic = _bleService->createCharacteristic(
      BLE_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY
  );
  _blePubSub.configureCharacteristic(_bleCharacteristic);
  _bleService->start();
  Serial.printf("[Serial] BLE service configured - Service UUID: %s, Char UUID: %s\n",
                BLE_SERVICE_UUID, BLE_CHAR_UUID);
}
#endif

// === Coordination with DiagnosticsService ===

void SerialService::suspendSerial() {
  if (_serialStarted && !_suspended) {
    Serial.println(F("[Serial] Suspending - DiagnosticsService taking control of Serial2"));
    Serial2.end();
    _serialStarted = false;
    _suspended = true;
  }
}

void SerialService::resumeSerial() {
  if (_suspended) {
    Serial.println(F("[Serial] Resuming - restarting Serial2"));
    _suspended = false;
    applySerialConfig();
  }
}
