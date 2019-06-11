<h3 align="center">Telnet server for ESP8266 / ESP32</h3>

---

<p align="center"> Cloning the serial port via Telnet.
    <br> 
</p>

<div align="center">

  [![GitHub Issues](https://img.shields.io/github/issues/yasheena/telnetspy.svg)](https://github.com/yasheena/telnetspy/issues)
  [![GitHub Pull Requests](https://img.shields.io/github/issues-pr/yasheena/telnetspy.svg)](https://github.com/yasheena/telnetspy/pulls)

</div>

## 📝 Table of Contents
- [Description](#description)
- [Usage](#usage)
  - [General](#general)
  - [Functions](#functions)
- [License](#license)


## 🎈 Description <a name = "description"></a>

- This module allows you to do "debugging over the air". So if you already use ArduinoOTA, this is a helpful extension for wireless development. Use ```TelnetSpy``` instead of ```Serial``` to send data to the serial port and a copy to a Telnet connection. 
- There is a circular buffer which allows to store the data while the Telnet connection is not established. So it's possible to collect data even when the WiFi and Telnet connections are not yet established.
- It's also possible to create a Telnet session only if it is neccessary: then you will get the already collected data as far as it is still stored in the circular buffer. Data sent from Telnet terminal to ESP8266 / ESP32 will be handled as data received by serial port.
- It is also possible to use more than one instance of TelnetSpy. For example - To send control information on the first instance and data dumps on the second instance.

## 🚀 Usage <a name = "usage"></a>

### General <a name = "general"></a>

Add the following line to your sketch:
```
#include <TelnetSpy.h>
TelnetSpy LOG;
```

Add the following line to your initialisation block ```void setup()```:
```
LOG.begin();
```

Add the following line at the beginning of your main loop ```void loop()```:
```
LOG.handle();
```

### Use the following functions of the TelnetSpy object to modify behavior <a name = "functions"></a>

1. [void setPort()](#setPort)
2. [void setWelcomeMsg()](#setWelcomeMsg)
3. [void setRejectMsg()](#setRejectMsg)
4. [void setMinBlockSize()](#setMinBlockSize)
5. [void setCollectingTime()](#setCollectingTime)
6. [void setMaxBlockSize()](#setMaxBlockSize)
7. [bool setBufferSize()](#setBufferSize)
8. [uint16_t getBufferSize()](#getBufferSize)
9. [void setStoreOffline()](#setStoreOffline)
10. [bool getStoreOffline()](#getStoreOffline)
11. [void setPingTime()](#setPingTime)
12. [void setSerial()](#setSerial)
14. [bool isClientConnected()](#isClientConnected)
14. [void setCallbackOnConnect()](#setCallbackOnConnect)
15. [void setCallbackOnDisconnect()](#setCallbackOnDisconnect)

---

### 1. void setPort(uint16_t portToUse) <a name = "setPort"></a>

Change the port number of this Telnet server. If a client is already connected, it will be disconnected.

Default: 23

```
void setPort(uint16_t portToUse)
```

### 2. void setWelcomeMsg(char* msg) <a name = "setWelcomeMsg"></a>

Change the message which will be sent to the Telnet client after a session is established.

Default: "Connection established via TelnetSpy.\n"

```
void setWelcomeMsg(char* msg)
```

### 3. void setRejectMsg(char* msg) <a name = "setRejectMsg"></a>

Change the message which will be sent to the Telnet client if another session is already established.

Default: "TelnetSpy: Only one connection possible.\n"

```
void setRejectMsg(char* msg)
```

### 4. void setMinBlockSize(uint16_t minSize) <a name = "setMinBlockSize"></a>

Change the amount of characters to collect before sending a Telnet block.

Default: 64 

```
void setMinBlockSize(uint16_t minSize)
```

### 5. void setCollectingTime(uint16_t colTime) <a name = "setCollectingTime"></a>

Change the time (in ms) to wait before sending a Telnet block if it's size is less than <minSize> (defined by ```setMinBlockSize```).

Default: 100

```
void setCollectingTime(uint16_t colTime)
```

### 6. void setMaxBlockSize(uint16_t maxSize) <a name = "setMaxBlockSize"></a>

Change the maximum size of the Telnet packets to send.

Default: 512

```
void setMaxBlockSize(uint16_t maxSize)
```

### 7. bool setBufferSize(uint16_t newSize) <a name = "setBufferSize"></a>

Change the size of the ring buffer. Set it to ```0``` to disable buffering. Changing size tries to preserve the already collected data. If the new buffer size is too small, only the latest data will be preserved. Returns ```false``` if the requested buffer size cannot be set.

Default: 3000

```
bool setBufferSize(uint16_t newSize)
```

### 8. uint16_t getBufferSize() <a name = "getBufferSize"></a>

This function returns the actual size of the ring buffer.

```
uint16_t getBufferSize()
```

### 9. void setStoreOffline(bool store) <a name = "setStoreOffline"></a>

Enable / disable storing new data in the ring buffer if no Telnet connection is established. This function allows you to store important data only. You can do this by disabling ```storeOffline``` for sending unimportant data.

Default: true

```
void setStoreOffline(bool store)
```

### 10. bool getStoreOffline() <a name = "getStoreOffline"></a>

Get actual state of storing data when offline.

```
bool getStoreOffline()
```

### 11. void setPingTime(uint16_t pngTime) <a name = "setPingTime"></a>

If no data is sent via TelnetSpy, the detection of a disconnected client has a long timeout. Use ```setPingTime``` to define the time (in ms) without traffic after which a ping aka a ```chr(0)``` is sent to the Telnet client to detect a disconnect earlier. Use ```0``` as parameter to disable pings.

Default: 1500  

```
void setPingTime(uint16_t pngTime)
```

### 12. void setSerial(HardwareSerial* usedSerial) <a name = "setSerial"></a>

Set the serial port you want to use with this object (especially for ESP32) or ```NULL``` if no serial port should be used (Telnet only).

Default: Serial

```
void setSerial(HardwareSerial* usedSerial)
```

### 13. bool isClientConnected() <a name = "isClientConnected"></a>

This function returns true if a Telnet client is connected.

```
bool isClientConnected()
```

### 14. void setCallbackOnConnect(void (*callback)()) <a name = "setCallbackOnConnect"></a>

This function installs a callback function which will be called on every Telnet connect of this object (except rejected connect tries). Use ```NULL``` to remove the callback.

Default: NULL

```
void setCallbackOnConnect(void (*callback)())
```

### 15. void setCallbackOnDisconnect(void (*callback)()) <a name = "setCallbackOnDisconnect"></a>

This function installs a callback function which will be called on every Telnet disconnect of this object (except rejected connect tries). Use ```NULL``` to remove the callback.

Default: NULL

```
void setCallbackOnDisconnect(void (*callback)())
```

## 💡 Hint <a name = "hint"></a>

Add the following lines to your sketch:

```
TelnetSpy SerialAndTelnet;
#define SERIAL SerialAndTelnet
// #define SERIAL Serial
```

Replace ```Serial``` with ```SERIAL``` in your sketch. Now you can switch between serial only and serial with Telnet by only changing the comments of the defines.
 
## 🔔 Important <a name = "important"></a>

- To connect to the Telnet server you have to:
    1. Establish the WiFi connection.
    2. Execute ```SerialAndTelnet.begin(WhatEverYouWant)```.
 
💡 **Note:** The order is not important.

- Everything you do with ```Serial```, you can do with ```TelnetSpy``` too. But remember: Transfering data also via Telnet will need more performance than the serial port only. So time critical things may be influenced.

- It is not possible to establish more than one Telnet connection at the same time. But it's possible to use more than one instance of TelnetSpy.

- If you have problems with low memory, you may reduce the value of the ```define TELNETSPY_BUFFER_LEN``` for a smaller ring buffer on initialisation.    

- Usage of ```void setDebugOutput(bool)``` to enable / disable of capturing of os_print calls when you have more than one TelnetSpy instance: That TelnetSpy object will handle this functionality where you used ```setDebugOutput``` at last.
On default, TelnetSpy has the capturing of OS_print calls enabled. So if you have more instances the last created instance will handle the capturing. 
 
## 📖 License <a name = "license"></a>

This library is open-source and licensed under the [MIT license](http://opensource.org/licenses/MIT).

Do whatever you like with it, but contributions are appreciated!
