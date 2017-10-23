#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TelnetSpy.h>

const char* ssid = ".....";
const char* password = ".....";

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

void setup() {
  SerialAndTelnet.setWelcomeMsg("Welcome to the TelnetSpy example\n\n");
  SERIAL.begin(74880);
  delay(100); // Wait for serial port
  SERIAL.print("\n\nConnecting to WiFi ");
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
    Serial.println("\nEnd");
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
  
  SERIAL.println("\nType 'C' for WiFi connect.\nType 'D' for WiFi disconnect.\nType 'R' for WiFi reconnect.");
  SERIAL.println("All other chars will be echoed. Play around...\n");
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
      case 'C':
        SERIAL.print("\nConnecting ");
        WiFi.begin(ssid, password);
        waitForConnection();
        break;
      case 'D':
        SERIAL.print("\nDisconnecting ...");
        WiFi.disconnect();
        waitForDisconnection();
        break;
      case 'R':
        SERIAL.print("\nReconnecting ");
        WiFi.reconnect();
        waitForDisconnection();
        waitForConnection();
        break;
      default:
        SERIAL.print(c);
        break;
    }
  }
}
