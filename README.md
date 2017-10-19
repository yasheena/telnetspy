# TELNET SERVER FOR ESP8266

Cloning the serial port via Telnet.

## DESCRIPTION

This module allows you "Debugging over the air". So if you already use ArduinoOTA this is a helpful extension for wireless development. Use "SerialAndTelnet" instead of "Serial" to send data to the serial port and a copy to a telnet connection. There is a circular buffer which allows to store the date before the telnet connection is established. So its possible to collect data even the Wifi and Telnet connections are still not established. Its also possible to create a telnet session only if it is neccessary: then you will get the already collected data as far as it is still stored in the circular buffer. Data send from telnet terminal to ESP8266 will be handled as data received by serial port.

## UASGE

Add the following line to your sketch:
```
#include "TelnetSpy.h"
```

Add the following line into your main loop ( void loop() ):
```
SerialAndTelnet.handle();
```

Install the "RingBuf" library (via library manager of Arduino IDE).

Add one of the following defines to your sketch, if you want to override the defaults (add them before the line #include "TelnetSpy.h"):
```
// Length of the data buffer for the telnet transfer
#define TELNETSPY_BUFFER_LEN 3000
// Collect TELNETSPY_MIN_BLOCK_SIZE characters before sending a telnet block
#define TELNETSPY_MIN_BLOCK_SIZE 64
// Wait TELNETSPY_COLLECTING_TIME ms before sending a telnet block if its
// size is less than TELNETSPY_MIN_BLOCK_SIZE
#define TELNETSPY_COLLECTING_TIME 100
// Send telnet data in chunks of maximum TELNETSPY_MAX_BLOCK_SIZE bytes. 
#define TELNETSPY_MAX_BLOCK_SIZE 512
```

## HINT

Add the following line to your sketch:
```
#define SERIAL SerialAndTelnet
```
Replace "Serial" with "SERIAL" in your sketch. Now you can switch between serial only and serial with telnet by changing the define only.
 
## IMPORTANT

To connect to the telnet server you have to:
 - establish the Wifi connection
 - execute "SerialAndTelnet.begin(WhatEverYouWant);"
The order is not important.
All you do with "Serial" you can also do with "SerialAndTelnet", but remember: Transfering data also via telnet will need more performance than the serial port only. So time critical things may be influenced. It is not possible to establish more than one telnet connection at the same time.

## License

This library is open-source and licensed under the [MIT license] (http://opensource.org/licenses/MIT). Do whatever you like with it, but contributions are appreciated.
