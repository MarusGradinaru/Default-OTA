#pragma once

#define NO_GLOBAL_ARDUINOOTA
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Preferences.h>

class DefaultOTAClass {
  public:
    void handleInit(const char* wifiSsid, const char* wifiPass, const char* otaHost, const char* otaPass,
      const char* wlogIP, uint16_t wlogPort, const char* fwVer = nullptr, unsigned long baud = 115200);
    void handleLoop();
    inline bool start() { return _start(true); }
    void stop();
    bool firmwareValidate();
    bool firmwareRollback();
    inline void setDefaultTestSessions(uint8_t tests) { _btest = tests; }
    inline int8_t getRemainingTestSessions() { return otaBoot; }
    inline bool isActive() { return _active; }
    inline bool isStarted() { return _started; }
    inline bool isTesting() { return otaBoot >= 0; }

  private:
    static constexpr char grpDefOta[]   = "def-ota";
    static constexpr char keyOtaBoot[]  = "ota-boot";
    static constexpr char keyOtaStart[] = "ota-start";

    uint8_t _btest = 3; int8_t otaBoot = -1;
    bool _inited = false, _active = false, _started = false;
    const char* _otaHost = nullptr;
    const char* _otaPass = nullptr;
    Preferences otaPrefs;
    ArduinoOTAClass ArduinoOTA;

    bool _start(bool keep);
    int8_t readOtaBoot();
    bool writeOtaBoot(int8_t value);
    bool readOtaStart();
    bool writeOtaStart(bool value);
    bool fwRollback();
    bool isCriticalReset(esp_reset_reason_t reason);
    void loopSafeMode(esp_reset_reason_t reason);
    void regOtaHandlers();
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_DEFAULTOTA)
  extern DefaultOTAClass DefaultOTA;
#endif

// TO DO:
//  - at the beginning, rollback should only be done if it is necessary, that is, if there is a valid boot
//  - handle the behavior when firmware rollback is enabled in the bootloader.

