#ifndef Features_h
#define Features_h

#define FT_ENABLED(feature) feature

// project feature off by default
#ifndef FT_PROJECT
#define FT_PROJECT 0
#endif

// security feature on by default
#ifndef FT_SECURITY
#define FT_SECURITY 1
#endif

// mqtt feature on by default
#ifndef FT_MQTT
#define FT_MQTT 1
#endif

// ntp feature on by default
#ifndef FT_NTP
#define FT_NTP 1
#endif

// mqtt feature on by default
#ifndef FT_OTA
#define FT_OTA 1
#endif

// upload firmware feature off by default
#ifndef FT_UPLOAD_FIRMWARE
#define FT_UPLOAD_FIRMWARE 0
#endif

// BLE feature ON by default for ESP32 IF there's enough flash
// Note: BLE adds ~600KB. On smaller partitions, disable with -D FT_BLE=0
#ifndef FT_BLE
#ifdef ESP32
#define FT_BLE 0  // Disabled by default due to flash constraints
#else
#define FT_BLE 0  // Always disabled on ESP8266
#endif
#endif


#endif
