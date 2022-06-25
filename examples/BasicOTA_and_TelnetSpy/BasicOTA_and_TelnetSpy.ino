#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <WiFiUdp.h>
#else // ESP32
  #include <WiFi.h>
  #include <ESPmDNS.h>
#endif
#include <ArduinoOTA.h>
#include <TelnetSpy.h>

const char* ssid = "yourSSID";
const char* password = "yourPassword";

TelnetSpy SerialAndTelnet;

//#define SER  Serial
#define SER  SerialAndTelnet

void waitForConnection() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SER.print(".");
  }
  SER.println(F(" Connected!"));
}

void waitForDisconnection() {
  while (WiFi.status() == WL_CONNECTED) {
    delay(500);
    SER.print(".");
  }
  SER.println(F(" Disconnected!"));
}

void telnetConnected() {
  SER.println(F("Telnet connection established."));
}

void telnetDisconnected() {
  SER.println(F("Telnet connection closed."));
}

void disconnectClientWrapper() {
  SerialAndTelnet.disconnectClient();
}

void setup() {
  SerialAndTelnet.setWelcomeMsg(F("Welcome to the TelnetSpy example\r\n\n"));
  SerialAndTelnet.setCallbackOnConnect(telnetConnected);
  SerialAndTelnet.setCallbackOnDisconnect(telnetDisconnected);
  SerialAndTelnet.setFilter(char(1), F("\r\nNVT command: AO\r\n"), disconnectClientWrapper);
  SER.begin(115200);
  delay(100); // Wait for serial port
//  SER.setDebugOutput(false);
  SER.print(F("\r\n\nConnecting to WiFi "));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  waitForConnection();

  // During updates "over the air" the telnet session will be closed.
  // So the operations of ArduinoOTA cannot be seen via telnet.
  // So we only use the standard "Serial" for logging.
  ArduinoOTA.onStart([]() {
    Serial.println(F("Start"));
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F(" \r\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  ArduinoOTA.begin();
  
  SER.println(F("Ready"));
  SER.print(F("IP address: "));
  SER.println(WiFi.localIP());
  
  SER.println(F("\r\nType 'C' for WiFi connect.\r\nType 'D' for WiFi disconnect.\r\nType 'R' for WiFi reconnect."));
  SER.println(F("Type 'X' or Ctrl-A for closing telnet session.\r\n"));
  SER.println(F("All other chars will be echoed. Play around...\r\n"));
  SER.println(F("The following 'Special Commands' (telnet NVT protocol) are supported:"));
  SER.println(F("  - Abort Output (AO) => closing telnet session."));
  SER.println(F("  - Interrupt Process (IP) => restart the ESP.\r\n"));
}

void loop() {
  SerialAndTelnet.handle();
  ArduinoOTA.handle();

  if (SER.available() > 0) {
    char c = SER.read();
    switch (c) {
      case '\r':
        SER.println();
        break;
      case '\n':
        break;
      case 'C':
        SER.print(F("\r\nConnecting "));
        WiFi.begin(ssid, password);
        waitForConnection();
        break;
      case 'D':
        SER.print(F("\r\nDisconnecting ..."));
        WiFi.disconnect();
        waitForDisconnection();
        break;
      case 'R':
        SER.print(F("\r\nReconnecting "));
        WiFi.reconnect();
        waitForDisconnection();
        waitForConnection();
        break;
      case 'X':
        SER.println(F("\r\nClosing telnet session..."));
        SerialAndTelnet.disconnectClient();
        break;
      default:
        SER.print(c);
        break;
    }
  }
}
