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

//#define SERIAL  Serial
#define SERIAL  SerialAndTelnet

void waitForConnection() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SERIAL.print(".");
  }
  SERIAL.println(" Connected!");
}

void waitForDisconnection() {
  while (WiFi.status() == WL_CONNECTED) {
    delay(500);
    SERIAL.print(".");
  }
  SERIAL.println(" Disconnected!");
}

void telnetConnected() {
  SERIAL.println("Telnet connection established.");
}

void telnetDisconnected() {
  SERIAL.println("Telnet connection closed.");
}

void disconnectClientWrapper() {
  SerialAndTelnet.disconnectClient();
}

void setup() {
  SerialAndTelnet.setWelcomeMsg("Welcome to the TelnetSpy example\r\n\n");
  SerialAndTelnet.setCallbackOnConnect(telnetConnected);
  SerialAndTelnet.setCallbackOnDisconnect(telnetDisconnected);
  SerialAndTelnet.setFilter(char(1), "\r\nNVT command: AO\r\n", disconnectClientWrapper);
  SERIAL.begin(115200);
  delay(100); // Wait for serial port
  SERIAL.setDebugOutput(false);
  SERIAL.print("\r\n\nConnecting to WiFi ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  waitForConnection();

  // During updates "over the air" the telnet session will be closed.
  // So the operations of ArduinoOTA cannot be seen via telnet.
  // So we only use the standard "Serial" for logging.
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(" \r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  SERIAL.println("Ready");
  SERIAL.print("IP address: ");
  SERIAL.println(WiFi.localIP());
  
  SERIAL.println("\r\nType 'C' for WiFi connect.\r\nType 'D' for WiFi disconnect.\r\nType 'R' for WiFi reconnect.");
  SERIAL.println("Type 'X' or Ctrl-A for closing telnet session.\r\n");
  SERIAL.println("All other chars will be echoed. Play around...\r\n");
  SERIAL.println("The following 'Special Commands' (telnet NVT protocol) are supported:");
  SERIAL.println("  - Abort Output (AO) => closing telnet session.");
  SERIAL.println("  - Interrupt Process (IP) => restart the ESP.\r\n");
}

void loop() {
  SerialAndTelnet.handle();
  ArduinoOTA.handle();

  if (SERIAL.available() > 0) {
    char c = SERIAL.read();
    switch (c) {
      case '\r':
        SERIAL.println();
        break;
      case '\n':
        break;
      case 'C':
        SERIAL.print("\r\nConnecting ");
        WiFi.begin(ssid, password);
        waitForConnection();
        break;
      case 'D':
        SERIAL.print("\r\nDisconnecting ...");
        WiFi.disconnect();
        waitForDisconnection();
        break;
      case 'R':
        SERIAL.print("\r\nReconnecting ");
        WiFi.reconnect();
        waitForDisconnection();
        waitForConnection();
        break;
      case 'X':
        SERIAL.println("\r\nClosing telnet session...");
        SERIAL.disconnectClient();
        break;
      default:
        SERIAL.print(c);
        break;
    }
  }
}
