#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>

#include <SmartLogger.h>
#include <DefaultOTA.h>

// Please use "include\credentials_template.h" to create your "credentials.h"
#include "credentials.h"

static constexpr char firm_ver[]     = "2.3.4";     //  <-- put here your firmware version

//Preferences Config;


// -------- Command Server ----------------------------------------

static constexpr uint16_t cmdRestart   = 0x3B91;
static constexpr uint16_t cmdValidate  = 0xC7A2;
static constexpr uint16_t cmdRollback  = 0x5ED4;
static constexpr uint16_t cmdOtaStart  = 0xF11F;
static constexpr uint16_t cmdOtaStop   = 0xF00F;
static constexpr uint16_t cmdResetWLog = 0xE2B8;

WebSocketsServer webSocket(ws_port);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* data, size_t len) {
  switch(type) {
    case WStype_DISCONNECTED:
      PrintFLn("[WSOCKET] Client [%u] disconnected.", num);
      break;
            
    case WStype_CONNECTED:
      PrintFLn("[WSOCKET] Client [%u] connected.", num);
      break;
          
    case WStype_BIN: if (len >= 2) {
      PrintF("[WSOCKET] Received data from client %u: ", num); PrintBuffLn(data, len);
      uint16_t cmdCode = data[0] | (data[1] << 8);
      switch (cmdCode) {

        case cmdRestart: 
          PrintLn("[WSOCKET] Restart requested. Restarting...");
          delay(500);
          ESP.restart();
          break;
         
        case cmdValidate:  DefaultOTA.firmwareValidate(); break; 
        case cmdRollback:  DefaultOTA.firmwareRollback(); break; 
        case cmdOtaStart:  DefaultOTA.start(); break; 
        case cmdOtaStop:   DefaultOTA.stop(); break; 
        case cmdResetWLog: ResetWiFiLogger(); break;
      }
    } break;
  }
}


void setup() { // ------------------- Setup Section -----------------------------------------------

  DefaultOTA.handleInit(wifi_ssid, wifi_pass, ota_host, ota_pass, wlog_ip, wlog_port, firm_ver);

  Print("[SETUP] Mouning LittleFS file system...");
  bool res = LittleFS.begin(true, "/LFS", 5, "littlefs"); PrintResult(res);
  if (!res) DefaultOTA.enterSafeModeLoop();

  Print("[SETUP] Starting WebSocket server...");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  PrintDoneLn();

  PrintLn("");
}

void loop() { // --------------------- Main Loop Section -----------------------------------------

  static wl_status_t wifiStat; static bool wifiBusy, wifiConnected, reconStarted = false;
  static int msg_count = 0, work_count = 0, recon_count = 500;

  wifiStat = WiFi.status();
  wifiConnected = (wifiStat == WL_CONNECTED);
  wifiBusy = (wifiStat == WL_IDLE_STATUS || wifiStat >= WL_DISCONNECTED);

  if (wifiConnected) {
    if (reconStarted) { recon_count = 500; reconStarted = false; }

    // handle WiFi dependent work here
    DefaultOTA.handleLoop();
    webSocket.loop();
    if (msg_count >= 250) { msg_count = 0; if (!DefaultOTA.isActive()) PrintLn("Hello from ESP32 !"); }  // at 5000 ms
    // -------------------------------

  } else if (!wifiBusy) {
    if (reconStarted) { recon_count = 0; reconStarted = false; }
    if (!reconStarted && recon_count >= 500) { WiFi.reconnect(); reconStarted = true; }  // at 10000 ms
  }

  // handle WiFi non-dependent work here
  if (work_count >= 25) { work_count = 0; /* do something */ }  // at 500 ms
  // -----------------------------------

  delay(20); msg_count++; work_count++; if (recon_count < 500) recon_count++;
}