#pragma once

#include <Arduino.h>

// your router 2.4 GHz SSID 
constexpr char wifi_ssid[]    = "...";

// your router 2.4 GHz Password
constexpr char wifi_pass[]    = "...";

// your logger server Local IP
constexpr char wlog_ip[]      = "192.168.0.X"; 

// your logger server Port - this should be changed in `utils\logger_server.py` too
constexpr uint16_t wlog_port  = 9000;

// your OTA Hostname - this should be changed in `platformio.ini` and `utils\command-panel.html` too
constexpr char ota_host[]     = "esp32-ota";

// your OTA Password -  - this should be changed in `platformio.ini` too
constexpr char ota_pass[]     = "supersecret";

// your command server Port -  - this should be changed in `utils\command-panel.html` too
constexpr uint16_t ws_port    = 5000;




