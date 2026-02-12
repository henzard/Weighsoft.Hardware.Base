# Changelog

All notable changes to Weighsoft Hardware Base will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **UART Mode Management**: Persistent mode switcher (Live Monitoring vs Diagnostics)
  - `UartModeService` for backend state management with flash persistence (`/config/uartMode.json`)
  - `UartModeSwitcher` React component for UI mode control
  - REST API (`/rest/uartMode`) and WebSocket (`/ws/uartMode`) endpoints
  - Automatic mode application on boot based on persisted setting
  - Mode switcher integrated into Serial and Diagnostics info pages
- **Hardware Coordination**: Enhanced mutual exclusion for Serial2 access
  - `stopAllTests()` method in DiagnosticsService for clean test termination
  - Automatic cleanup of diagnostic tests when switching to Live mode
  - SerialService properly resumes after diagnostics mode ends

### Changed
- Updated boot sequence to include UartModeService initialization ([x/10] now)
- Main.cpp now links UartModeService with Serial and Diagnostics services for coordination
- API documentation updated with UART Mode Management endpoints
- Frontend components refactored to support mode-aware rendering

## [0.2.0] - 2024-02-12

### Added
- **UART Diagnostics Project**: Complete hardware testing suite
  - Loopback Test: Verifies GPIO16-17 hardware functionality with echo packets
  - Baud Rate Detection: Automatically detects device baud rate (1200-115200)
  - Signal Quality Test: Measures latency, jitter, and packet loss
- Backend diagnostics service (`DiagnosticsService.h/cpp`, `DiagnosticsState.h`)
- REST endpoint: `/rest/diagnostics` for test control
- WebSocket endpoint: `/ws/diagnostics` for real-time test updates
- Frontend diagnostics UI with Material-UI components:
  - `DiagnosticsInfo.tsx`: Documentation and use cases
  - `LoopbackTest.tsx`: Real-time loopback test with success rate display
  - `BaudDetector.tsx`: Baud scanner with progress stepper
  - `SignalQuality.tsx`: Signal analysis with quality meter
- Comprehensive documentation: `DIAGNOSTICS-EXAMPLE.md` (534 lines)
- Software versioning system:
  - `src/version.h`: Backend version tracking
  - Version display in boot logs
  - Software versioning rule (`.cursor/rules/software-versioning.mdc`)
- Automatic version control rule (`.cursor/rules/version-control.mdc` updated)

### Changed
- Updated `main.cpp` boot sequence to [x/8] for diagnostics service
- Updated `API-REFERENCE.md` with diagnostics endpoints
- Updated `FILE-REFERENCE.md` with new diagnostics files
- Enhanced boot logging with version, build date/time, and API version
- Version control rule now enforces automatic commit/push after task completion

### Fixed
- None in this release

## [0.1.0] - 2024-01-10

### Added
- Initial serial monitoring service
- Serial2 (GPIO16/17) monitoring and streaming
- Configurable serial port settings (baud rate, data bits, stop bits, parity)
- Regex-based weight extraction from serial data
- REST API: `/rest/serial`
- WebSocket: `/ws/serial` for real-time data streaming
- MQTT publishing to `weighsoft/serial/{unique_id}/data`
- BLE notifications for serial data
- Serial configuration UI with live preview
- FSPersistence for serial configuration
- Comprehensive serial documentation: `SERIAL-EXAMPLE.md`
- LED example service for bidirectional control
- ESP8266-React framework integration
- Material-UI responsive frontend
- JWT authentication
- WiFi, MQTT, NTP, OTA configuration
- Comprehensive project documentation

### Changed
- N/A (initial release)

### Fixed
- Serial2 not initializing on boot
- Baud rate configuration not persisting after reboot
- Line ending parsing (now accepts both `\r` and `\n`)

---

## Version History

- **0.2.0** - UART Diagnostics + Software Versioning
- **0.1.0** - Initial Release (Serial Monitoring + LED Example)

## Upcoming

See GitHub Issues for planned features and improvements.
