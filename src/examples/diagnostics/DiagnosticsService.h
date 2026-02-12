#ifndef DiagnosticsService_h
#define DiagnosticsService_h

#include <HttpEndpoint.h>
#include <WebSocketTxRx.h>
#include <SettingValue.h>
#include <examples/diagnostics/DiagnosticsState.h>

#define DIAGNOSTICS_ENDPOINT_PATH "/rest/diagnostics"
#define DIAGNOSTICS_SOCKET_PATH "/ws/diagnostics"

// Use same GPIO pins as Serial2
#define DIAG_RX_PIN 16
#define DIAG_TX_PIN 17

class DiagnosticsService : public StatefulService<DiagnosticsState> {
 public:
  DiagnosticsService(AsyncWebServer* server, SecurityManager* securityManager);
  void begin();
  void loop();  // Must be called in main loop()

 private:
  HttpEndpoint<DiagnosticsState> _httpEndpoint;
  WebSocketTxRx<DiagnosticsState> _webSocket;

  String _rxBuffer;
  bool _serialStarted;
  unsigned long _lastTestTime;

  // Test-specific state
  unsigned long _loopbackLastSend;
  unsigned long _baudTestStartTime;
  unsigned long _signalTestStartTime;
  float* _latencyBuffer;  // For jitter calculation
  uint16_t _latencyBufferSize;

  // Test methods
  void runLoopbackTest();
  void runBaudScan();
  void runSignalQualityTest();

  // Helper methods
  void startSerial(uint32_t baud);
  void stopSerial();
  String readSerialLine();
  void calculateSignalQuality();
};

#endif
