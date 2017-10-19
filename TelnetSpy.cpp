/********************************/
/*                              */
/*  Project:    TelnetSpy       */
/*  Version:    1.00            */
/*  Date:       08.10.2017      */
/*                              */
/*  Author:     Wolfgang Mattis */
/*  Email:      w@wm0.eu        */
/*                              */
/********************************/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <RingBuf.h>
#include "TelnetSpy.h"

WiFiServer telnetServer(23);
WiFiClient serverClient;
RingBuf *telnetBuf = RingBuf_new(sizeof(char), TELNETSPY_BUFFER_LEN);

TelnetSpy::TelnetSpy() {
  started = false;
  listening = false;
  waitRef = 0xFFFFFFFF;
}
    
size_t TelnetSpy::write (uint8_t data) {
  if (telnetBuf) {
    if (telnetBuf->isFull(telnetBuf)) {
      if (serverClient && serverClient.connected()) {
        sendBlock();
      }
      if (telnetBuf->isFull(telnetBuf)) {
        char c;
        while (!telnetBuf->isEmpty(telnetBuf)) {
          telnetBuf->pull(telnetBuf, &c);
          if (c == '\n') {
            break;
          }
        }
      }
    }
    telnetBuf->add(telnetBuf, &data);
  } else {
    if (serverClient && serverClient.connected()) {
      serverClient.write(data);
    }
  }
  return Serial.write(data);
}
    
int TelnetSpy::available (void) {
  int avail = Serial.available();
  if (avail > 0) {
    return avail;
  }
  if (serverClient && serverClient.connected()) {
    return telnetAvailable();
  }
  return 0;
}

int TelnetSpy::read (void) {
  int val = Serial.read();
  if (val != -1) {
    return val;
  }
  if (serverClient && serverClient.connected()) {
    if (telnetAvailable()) {
      val = serverClient.read();
    }
  }
  return val;
}
    
int TelnetSpy::peek (void) {
  int val = Serial.peek();
  if (val != -1) {
    return val;
  }
  if (serverClient && serverClient.connected()) {
    if (telnetAvailable()) {
      val = serverClient.peek();
    }
  }
  return val;
}
    
void TelnetSpy::flush (void) {
  Serial.flush();
  if (serverClient && serverClient.connected()) {
    while (!telnetBuf->isEmpty(telnetBuf)) {
      sendBlock();
    }
    serverClient.flush();
  }
}

void TelnetSpy::begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin) {
  Serial.begin(baud, config, mode, tx_pin);
  started = true;
}

void TelnetSpy::end() {
  started = false;
  Serial.end();
  if (serverClient && serverClient.connected()) {
    serverClient.stop();
  }
}

void TelnetSpy::swap(uint8_t tx_pin) {
  Serial.swap(tx_pin);
}

void TelnetSpy::set_tx(uint8_t tx_pin) {
  Serial.set_tx(tx_pin);
}

void TelnetSpy::pins(uint8_t tx, uint8_t rx) {
  Serial.pins(tx, rx);
}

int TelnetSpy::availableForWrite(void) {
  return Serial.availableForWrite();
}

TelnetSpy::operator bool() const {
  return (bool) Serial;
}

void TelnetSpy::setDebugOutput(bool en) {
  Serial.setDebugOutput(en);
}

bool TelnetSpy::isTxEnabled(void) {
  return Serial.isTxEnabled();
}

bool TelnetSpy::isRxEnabled(void) {
  return Serial.isRxEnabled();
}

int TelnetSpy::baudRate(void) {
  return Serial.baudRate();
}

void TelnetSpy::sendBlock() {
  unsigned int len = telnetBuf->numElements(telnetBuf);
  if (len > TELNETSPY_MAX_BLOCK_SIZE) {
    len = TELNETSPY_MAX_BLOCK_SIZE;
  }
  uint8_t buf[len];
  for (int i = 0; i < len; i++) {
    telnetBuf->pull(telnetBuf, &buf[i]);
  }
  serverClient.write(buf, len);
  waitRef = 0xFFFFFFFF;
}

int TelnetSpy::telnetAvailable() {
  int n = serverClient.available();
  while (n > 0) {
    if (0xff == serverClient.peek()) {  // If esc char for telnet NVT protocol data remove that telegram:
      serverClient.read();  // Remove esc char
      n--;
      if (0xff == serverClient.peek()) {  // If esc sequence for 0xFF data byte...
        return n; // ...return info about available data (just this 0xFF data byte)
      }
      serverClient.read();  // Skip the rest of the telegram of the telnet NVT protocol data
      serverClient.read();
      n--;
      n--;
    } else {  // If next char is a normal data byte...
      return n;   // ...return info about available data
    }
  }
  return 0;
}

void TelnetSpy::handle() {
  if (!listening) {
    if (started) {
      if (WiFi.status() == WL_CONNECTED) {
        telnetServer.begin();
        telnetServer.setNoDelay(true);
        listening = true;
      }
    }
    return;
  }
  // Check for disconnected client
  if (serverClient && !serverClient.connected()) {
    serverClient.stop();
  }
  // Check for new client connection
  if (telnetServer.hasClient()) {
    if (!started || serverClient) {
      telnetServer.available().stop();
    } else {
      serverClient = telnetServer.available();
      serverClient.write("Connection established via TelnetSpy.\n");
      serverClient.flush();
    }
  }
  if (serverClient && serverClient.connected() && !telnetBuf->isEmpty(telnetBuf)) {
    unsigned int len = telnetBuf->numElements(telnetBuf);
    if (len >= TELNETSPY_MIN_BLOCK_SIZE) {
      sendBlock();
    } else {
      unsigned long m = millis() & 0x7FFFFFF;
      if (waitRef == 0xFFFFFFFF) {
        waitRef = m + TELNETSPY_COLLECTING_TIME;
        if (waitRef > 0x7FFFFFFF) {
          waitRef -= 0x80000000;
        }
      } else {
        if (!((waitRef < 0x20000000) && (m > 0x60000000)) && (m >= waitRef)) {
          sendBlock();
        }
      }
    }
  }
}

TelnetSpy SerialAndTelnet;
