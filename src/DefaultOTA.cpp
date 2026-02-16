#include "DefaultOTA.h"
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include <SmartLogger.h>

//---------- Public Section -------------------------------------

void DefaultOTAClass::handleInit(const char* wifiSsid, const char* wifiPass, const char* otaHost,
    const char* otaPass, const char* wlogIP, uint16_t wlogPort, const char* fwVer, unsigned long baud) {
  if (_inited) return; _inited = true; _otaHost = otaHost; _otaPass = otaPass;

  // firmware rollback handler
  otaBoot = readOtaBoot();
  if (otaBoot >= 0 && !writeOtaBoot(otaBoot-1)) Halt();
  if (otaBoot == 0) if (fwRollback()) ESP.restart(); else Halt();
  esp_reset_reason_t reason = esp_reset_reason();

  // starting serial logger if available
  DebugSBlock( if (baud > 0) StartSerialLogger(baud); );

  // connecting to WiFi network
  if (wifiSsid == nullptr || strlen(wifiSsid) == 0 || wifiPass == nullptr || strlen(wifiPass) == 0)
    { PrintLn("[SETUP] Invalid WiFi credentials !"); Halt(); }
  Print("[SETUP] Connecting to WiFi...");
  WiFi.persistent(false); WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA); WiFi.begin(wifiSsid, wifiPass); 
  while (WiFi.waitForConnectResult(120000) != WL_CONNECTED) { Print("."); delay(10000); }
  PrintDoneLn();
  
  // starting WiFi logger if available
  DebugWBlock(
    if (!StartWifiLogger(wlogIP, wlogPort)) 
      PrintLn("[SETUP] Invalid WiFi logger address !");
  );

  // show system info
  DebugBlock(  
    if (fwVer != nullptr && strlen(fwVer) > 0) PrintFirmInfo("\r\n[SETUP]", fwVer);
    PrintFLn("[SETUP] Framework: Arduino v%s bassed on ESP-IDF %s", ESP_ARDUINO_VERSION_STR, IDF_VER);   
    if (otaBoot >= 1) PrintFLn("[SETUP] You have %d session(s) left to evaluate the new firmware.", otaBoot);
      else PrintLn("[SETUP] This firmware is validated as stable."); 
    PrintRReason("[SETUP] Restart reason: %s", reason);
    PrintIp("[SETUP] Local IP: %s", WiFi.localIP()); 
  );

  // start OTA service if it's enabled 
  regOtaHandlers(); if (readOtaStart()) if (!_start(false)) Halt();

  // Safe Mode handler
  if (otaBoot == -1 && isCriticalReset(reason)) loopSafeMode(reason);
}

void DefaultOTAClass::handleLoop() {
  if (_inited && WiFi.status() == WL_CONNECTED) ArduinoOTA.handle();
}

bool DefaultOTAClass::_start(bool keep) {  // when called from Public, "keep" is alway true
  if (!_inited) return false; if (_started) return true;
  if (_otaHost == nullptr || strlen(_otaHost) == 0)
    { PrintLn("[SETUP] Invalid OTA host name !"); return false; }
  Print("[SETUP] Starting OTA..."); ArduinoOTA.setHostname(_otaHost);
  if (_otaPass != nullptr && strlen(_otaPass) > 0) ArduinoOTA.setPassword(_otaPass);
  ArduinoOTA.setRebootOnSuccess(false);
  ArduinoOTA.begin(); PrintDoneLn(); 
  if (keep) {
    Print("[SETUP] Updating settings...");
    bool res = writeOtaStart(true); PrintResult(res); }
  _started = true; return true;   
}

void DefaultOTAClass::stop() {
  if (!_inited || !_started || _active) return;
  Print("[SETUP] Stopping OTA...");
  ArduinoOTA.end(); PrintResult(true);
  Print("[SETUP] Updating settings...");
  bool res = writeOtaStart(false); PrintResult(res);
  _started = false;
}

bool DefaultOTAClass::firmwareValidate() {
  if (!_inited || !isTesting() || _active) return false;
  Print("[OTA] Validating current firmware...");
  bool res = writeOtaBoot(-1); PrintResult(res);
  return res;
}

bool DefaultOTAClass::firmwareRollback() {
  if (!_inited || !isTesting() || _active) return false;
  Print("[OTA] Rolling back to previous firmware...");
  bool res = fwRollback(); PrintResult(res);
  return res;
} 


//---------- Private Section ------------------------------------

int8_t DefaultOTAClass::readOtaBoot(){
  if (!otaPrefs.begin(grpDefOta, true)) return -1;
  int8_t res = otaPrefs.getChar(keyOtaBoot, -1);
  otaPrefs.end();
  return res;
}

bool DefaultOTAClass::writeOtaBoot(int8_t value) {
  if (!otaPrefs.begin(grpDefOta, false)) return false;
  bool res = otaPrefs.putChar(keyOtaBoot, value);
  otaPrefs.end();
  return res;
}

bool DefaultOTAClass::readOtaStart(){
  if (!otaPrefs.begin(grpDefOta, true)) return true;
  bool res = otaPrefs.getBool(keyOtaStart, true);
  otaPrefs.end();
  return res;
}

bool DefaultOTAClass::writeOtaStart(bool value) {
  if (!otaPrefs.begin(grpDefOta, false)) return false;
  bool res = otaPrefs.putBool(keyOtaStart, value);
  otaPrefs.end();
  return res;
}

bool DefaultOTAClass::fwRollback() {  
  // works if the partition table has only 2 ota partitions
  const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
  if (next == NULL) return false;
  return esp_ota_set_boot_partition(next) == ESP_OK;
}

bool DefaultOTAClass::isCriticalReset(esp_reset_reason_t reason) {
  return !(reason == ESP_RST_POWERON || reason == ESP_RST_EXT || 
    reason == ESP_RST_SW || reason == ESP_RST_DEEPSLEEP);
}

void DefaultOTAClass::loopSafeMode(esp_reset_reason_t reason) {
  PrintRReason("[SETUP] Entering Safe Mode... Reason: %s", reason);
  if (!_started) if (!_start(false)) { PrintLn("[SETUP] System halted."); Halt(); }
  PrintLn("[SETUP] Only OTA firmware upload available.");
  while(true) {
    if (WiFi.status() == WL_CONNECTED) { ArduinoOTA.handle(); delay(20); }
    else { WiFi.reconnect(); if (WiFi.waitForConnectResult(120000) != WL_CONNECTED) delay(10000); }
  }
}

void DefaultOTAClass::regOtaHandlers() {
  ArduinoOTA.onStart([this]() {
    _active = true;
    PrintLn("\r\n[OTA] Upload started...");
  });

  ArduinoOTA.onEnd([this]() {
    PrintLn("[OTA] The upload was completed successfully.");
    Print("[OTA] Updating ota-boot NVS flag...");
    bool res = writeOtaBoot(_btest); PrintResult(res);
    if (res) { PrintLn("[OTA] Restarting..."); delay(100); ESP.restart(); }
    _active = false;
    PrintLn("");
  });

  ArduinoOTA.onError([this](ota_error_t error) {
    PrintFLn("[OTA] Upload ended with Error %u.\r\n", error);
    _active = false;
  });
}


//---------- Global Section ------------------------------------

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_DEFAULTOTA)
  DefaultOTAClass DefaultOTA;
#endif

