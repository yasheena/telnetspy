/*
 * TELNET SERVER FOR ESP8266 / ESP32
 * Cloning the serial port via Telnet.
 *
 * Written by Wolfgang Mattis (arduino@wm0.eu).
 * Version 1.4 / June 25, 2022.
 * MIT license, all text above must be included in any redistribution.   
 */

/*
 * DESCRIPTION
 *
 * This module allows you "Debugging over the air". So if you already use
 * ArduinoOTA this is a helpful extension for wireless development. Use 
 * "TelnetSpy" instead of "Serial" to send data to the serial port and a copy
 * to a telnet connection. There is a circular buffer which allows to store the
 * data while the telnet connection is not established. So its possible to
 * collect data even when the Wifi and Telnet connections are still not
 * established. Its also possible to create a telnet session only if it is
 * neccessary: then you will get the already collected data as far as it is
 * still stored in the circular buffer. Data send from telnet terminal to
 * ESP8266 / ESP32 will be handled as data received by serial port. It is also
 * possible to use more than one instance of TelnetSpy, for example to send
 * control information on the first instance and data dumps on the second
 * instance.
 *
 * USAGE
 *
 * Add the following line to your sketch:
 *		#include <TelnetSpy.h>
 *		TelnetSpy LOG;
 *
 * Add the following line to your initialisation block ( void setup() ):
 *		LOG.begin();
 *
 * Add the following line at the beginning of your main loop ( void loop() ):
 *		LOG.handle();
 *
 * Use the following functions of the TelnetSpy object to modify behavior
 *
 * Change the port number of this telnet server. If a client is already
 * connected it will be disconnected.
 * Default: 23
 *		void setPort(uint16_t portToUse);
 *		 
 * Change the message which will be send to the telnet client after a session
 * is established.
 * Default: "Connection established via TelnetSpy.\n"
 *		void setWelcomeMsg(const char* msg);
 *		void setWelcomeMsg(const String& msg);
 *
 * Change the message which will be send to the telnet client if another
 * session is already established.
 * Default: "TelnetSpy: Only one connection possible.\n"
 *		void setRejectMsg(const char* msg);
 *		void setRejectMsg(const String& msg);
 *
 * Change the amount of characters to collect before sending a telnet block.
 * Default: 64 
 *		void setMinBlockSize(uint16_t minSize);
 *
 * Change the time (in ms) to wait before sending a telnet block if its size is
 * less than <minSize> (defined by setMinBlockSize).
 * Default: 100
 *		void setCollectingTime(uint16_t colTime);
 *
 * Change the maximum size of the telnet packets to send.
 * Default: 512
 *		void setMaxBlockSize(uint16_t maxSize);
 *
 * Change the size of the transmit buffer. Set it to 0 to disable buffering.
 * If buffering is disabled, the system's debug output (see setDebugOutput)
 * cannot be send via telnet, it will be send to serial output only.
 * Changing size tries to preserve the already collected data. If the new
 * buffer size is too small the youngest data will be preserved only. Returns
 * false if the requested buffer size cannot be set.
 * Default: 3000
 *		bool setBufferSize(uint16_t newSize);
 *
 * This function returns the actual size of the transmit buffer.
 *		uint16_t getBufferSize();
 *
 * Enable / disable storing new data in the transmit buffer if no telnet
 * connection is established. This function allows you to store important data
 * only. You can do this by disabling "storeOffline" for sending less important
 * data.
 * Default: true
 *		void setStoreOffline(bool store);
 *
 * Get actual state of storing data in the transmit buffer when offline.
 *		bool getStoreOffline();
 *
 * If no data is sent via TelnetSpy the detection of a disconnected client has
 * a long timeout. Use setPingTime to define the time (in ms) without traffic
 * after which a ping (chr(0)) is sent to the telnet client to detect a
 * disconnect earlier. Use 0 as parameter to disable pings.
 * Default: 1500  
 *		void setPingTime(uint16_t pngTime);
 *
 * Change the size of the receive buffer. Set it to 0 to disable buffering in
 * TelnetSpy (there is still a buffer in the underlayed WifiClient component).
 * Returns false if the requested buffer size cannot be set.
 * - If the receive buffer is used and it is full, additional received data
 * will be lost. But all telnet NVT protocol data and the "filter character" is
 * still handled (see "setFilter" and the NVT callbacks below).
 * - If no receive buffer is used and the received characters are not retrieved
 * by your app, the handling of the NVT protocol and the "filter character"
 * will not work. If no receive buffer is used, you cannot receive the code
 * 0xff (it will be lost because of a limitation of the WiFiAPI).
 * Default: 64
 *		bool setRecBufferSize(uint16_t newSize);
 *
 * This function returns the actual size of the receive buffer.
 *		uint16_t getRecBufferSize();
 *
 * Set the serial port you want to use with this object (especially for ESP32)
 * or NULL if no serial port should be used (telnet only).
 * Default: Serial
 *		void setSerial(HardwareSerial* usedSerial);
 *
 * This function returns true, if a telnet client is connected.
 *		bool isClientConnected();
 *
 * This function installs a callback function which will be called on every
 * telnet connect of this object (except rejected connect tries). Use NULL to
 * remove the callback.
 * Default: NULL
 *		void setCallbackOnConnect(void (*callback)());
 *
 * This function installs a callback function which will be called on every
 * telnet disconnect of this object (except rejected connect tries). Use NULL
 * to remove the callback.
 * Default: NULL
 *		void setCallbackOnDisconnect(void (*callback)());
 *
 * This function disconnects an active client connection.
 *      void disconnectClient();
 *
 * This function clears the transmit buffer of TelnetSpy, so all waiting data
 * to send via a telnet connection will be discard.
 *      void clearBuffer();
 *
 * This function allows to filter the character given by "ch" out of the
 * receiving telnet data stream. If this character is detected, the following
 * happens:
 *  - If a "msg" is given (not NULL), this message will be send back via the
 *      telnet connection.
 *  - If the "callback" is set (not NULL), the given function is called.
 *      void setFilter(char ch, const char* msg, void (*callback());
 *      void setFilter(char ch, const String& msg, void (*callback());
 *
 * This function returns the actual filter character (0 => not set).
 *      char getFilter();
 *
 * There is a rudimentary implementation of the telnet NVT protocol (see
 * RFC854). You can use this functions i.e. in PuTTY via its menu "Special
 * Command". The following functions can set callbacks to modify the behaviour.
 *
 * This function installs a callback function which will be called whenever the
 * telnet command "BRK" (Break) is received. Use NULL to remove the callback.
 * Default: NULL
 *		void setCallbackOnNvtBRK(void (*callback)());
 *
 * This function installs a callback function which will be called whenever
 * the telnet command "IP" (Interrupt Process) is received. Use NULL to remove
 * the callback.
 * Default: 1 (=> ESP.restart will be called)
 *		void setCallbackOnNvtIP)(void (*callback)());
 *
 * This function installs a callback function which will be called whenever
 * the telnet command "AO" (Abort Output) is received. Use NULL to remove the
 * callback.
 * Default: 1 (=> disconnectClient of TelnerSpy will be called)
 *		void setCallbackOnNvtAO)(void (*callback)());
 *
 * This function installs a callback function which will be called whenever
 * the telnet command "AYT" (Are you there) is received. Use NULL to remove the
 * callback.
 * Default: NULL
 *		void setCallbackOnNvtAYT)(void (*callback)());
 *
 * This function installs a callback function which will be called whenever
 * the telnet command "EC" (Erase Character) is received. Use NULL to remove
 * the callback.
 * Default: NULL
 *		void setCallbackOnNvtEC)(void (*callback)());
 *
 * This function installs a callback function which will be called whenever
 * the telnet command "EL" (Erase Line) is received. Use NULL to remove the
 * callback.
 * Default: NULL
 *		void setCallbackOnNvtEL)(void (*callback)());
 *
 * This function installs a callback function which will be called whenever
 * the telnet command "GA" (Go Ahead) is received. Use NULL to remove the
 * callback.
 * Default: NULL
 *		void setCallbackOnNvtGA)(void (*callback)());
 *
 * This function installs a callback function which will be called whenever
 * the telnet commands "WILL", "WON'T", "DO" or "DON'T" are received. Use NULL
 * to remove the callback.
 * Default: NULL
 *		void setCallbackOnNvtWWDD(void (*callback)(char command, char option));
 *
 * HINT
 *
 * Add the following lines to your sketch:
 *		#define SERIAL TelnetSpy
 *		//#define SERIAL Serial
 *
 * Replace "Serial" with "SERIAL" in your sketch. Now you can switch between
 * serial only and serial with telnet by changing the comments of the defines
 * only.
 *
 * IMPORTANT
 *
 * To connect to the telnet server you have to:
 *	- establish the Wifi connection
 *	- execute "TelnetSpy.begin(WhatEverYouWant);"
 *
 * The order is not important.
 *
 * All you do with "Serial" you can also do with "TelnetSpy", but remember:
 * Transfering data also via telnet will need more performance than the serial
 * port only. So time critical things may be influenced.
 *
 * It is not possible to establish more than one telnet connection at the same
 * time. But its possible to use more than one instance of TelnetSpy.
 *
 * If you have problems with low memory you may reduce the value of the define
 * TELNETSPY_BUFFER_LEN for a smaller ring buffer on initialisation.    
 *
 * Usage of void setDebugOutput(bool) to enable / disable of capturing of
 * os_print calls when you have more than one TelnetSpy instance: That
 * TelnetSpy object will handle this functionallity where you used
 * setDebugOutput at last. On default TelnetSpy has the capturing of OS_print
 * calls enabled. So if you have more instances the last created instance will
 * handle the capturing.
 */

#ifndef TelnetSpy_h
#define TelnetSpy_h

#define TELNETSPY_BUFFER_LEN 3000
#define TELNETSPY_MIN_BLOCK_SIZE 64
#define TELNETSPY_COLLECTING_TIME 100
#define TELNETSPY_MAX_BLOCK_SIZE 512
#define TELNETSPY_PING_TIME 1500
#define TELNETSPY_PORT 23
#define TELNETSPY_CAPTURE_OS_PRINT true
#define TELNETSPY_WELCOME_MSG "Connection established via TelnetSpy.\r\n"
#define TELNETSPY_REJECT_MSG "TelnetSpy: Only one connection possible.\r\n"
#define TELNETSPY_REC_BUFFER_LEN 64

#ifdef ESP8266
#include <ESP8266WiFi.h>
// empty defines, so on ESP8266 nothing will be changed
#define CRITCAL_SECTION_MUTEX
#define CRITCAL_SECTION_START
#define CRITCAL_SECTION_END
#define WIFI_MODE_NULL  NULL_MODE
#define WIFI_MODE_STA   STATION_MODE
#define WIFI_MODE_AP    SOFTAP_MODE
#define WIFI_MODE_APSTA STATIONAP_MODE
#else // ESP32
#include <WiFi.h>
// add spinlock for ESP32
#define CRITCAL_SECTION_MUTEX portMUX_TYPE AtomicMutex = portMUX_INITIALIZER_UNLOCKED;
// Non-static Data Member Initializers, see: https://web.archive.org/web/20160316174223/https://blogs.oracle.com/pcarlini/entry/c_11_tidbits_non_static
#define CRITCAL_SECTION_START portENTER_CRITICAL(&AtomicMutex);
#define CRITCAL_SECTION_END portEXIT_CRITICAL(&AtomicMutex);
#endif
#include <WiFiClient.h>

class TelnetSpy : public Stream {
	public:
		TelnetSpy();
		~TelnetSpy();
		void handle(void);
		void setPort(uint16_t portToUse);
		void setWelcomeMsg(const char* msg);
		void setWelcomeMsg(const String& msg);
		void setRejectMsg(const char* msg);
		void setRejectMsg(const String& msg);
		void setMinBlockSize(uint16_t minSize);
		void setCollectingTime(uint16_t colTime);
		void setMaxBlockSize(uint16_t maxSize);
		bool setBufferSize(uint16_t newSize);
		uint16_t getBufferSize();
		void setStoreOffline(bool store);
		bool getStoreOffline();
		void setPingTime(uint16_t pngTime);
		bool setRecBufferSize(uint16_t newSize);
		uint16_t getRecBufferSize();
#if ARDUINO_USB_CDC_ON_BOOT
		void setSerial(USBCDC* usedSerial);
#else
		void setSerial(HardwareSerial* usedSerial);
#endif
		bool isClientConnected();
		void setCallbackOnConnect(void (*callback)());
		void setCallbackOnDisconnect(void (*callback)());
        void disconnectClient();
        void clearBuffer();
        void setFilter(char ch, const char* msg, void (*callback)());
        void setFilter(char ch, const String& msg, void (*callback)());
        char getFilter();
		void setCallbackOnNvtBRK(void (*callback)());
		void setCallbackOnNvtIP(void (*callback)());
		void setCallbackOnNvtAO(void (*callback)());
		void setCallbackOnNvtAYT(void (*callback)());
		void setCallbackOnNvtEC(void (*callback)());
		void setCallbackOnNvtEL(void (*callback)());
		void setCallbackOnNvtGA(void (*callback)());
		void setCallbackOnNvtWWDD(void (*callback)(char command, char option));
		// Functions offered by HardwareSerial class:
#ifdef ESP8266
		void begin(unsigned long baud) { begin(baud, SERIAL_8N1, SERIAL_FULL, 1); }
		void begin(unsigned long baud, SerialConfig config) { begin(baud, config, SERIAL_FULL, 1); }
		void begin(unsigned long baud, SerialConfig config, SerialMode mode) { begin(baud, config, mode, 1); }
		void begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin);
#else	// ESP32
		void begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1, bool invert=false);
#endif
		void end();
#ifdef ESP8266
		void swap() { swap(1); }
		void swap(uint8_t tx_pin);
		void set_tx(uint8_t tx_pin);
		void pins(uint8_t tx, uint8_t rx);
		bool isTxEnabled(void);
		bool isRxEnabled(void);
#endif
		int available(void) override;
		int peek(void) override;
		int read(void) override;
		int availableForWrite(void);
		void flush(void) override;
		void debugWrite(uint8_t);
		size_t write(uint8_t) override;
		inline size_t write(unsigned long n) { return write((uint8_t) n); }
		inline size_t write(long n) { return write((uint8_t) n); }
		inline size_t write(unsigned int n) { return write((uint8_t) n); }
		inline size_t write(int n) { return write((uint8_t) n); }
		using Print::write;
		operator bool() const;
		void setDebugOutput(bool);
		uint32_t baudRate(void);

	protected:
		CRITCAL_SECTION_MUTEX
		void sendBlock(void);
		void addTelnetBuf(char c);
		char pullTelnetBuf();
		char peekTelnetBuf();
		int telnetAvailable();
        void writeRecBuf(char c);
        void checkReceive();
		WiFiServer* telnetServer;
		WiFiClient client;
		uint16_t port;
#if ARDUINO_USB_CDC_ON_BOOT
		USBCDC* usedSer;
#else
		HardwareSerial* usedSer;
#endif
		bool storeOffline;
		bool started;
		bool listening;
		bool firstMainLoop;
		unsigned long waitRef;
		unsigned long pingRef;
		uint16_t pingTime;
        bool nvtDetected;
		char* welcomeMsg;
		char* rejectMsg;
        char filterChar;
        char* filterMsg;
        void (*filterCallback)();
		uint16_t minBlockSize;
		uint16_t collectingTime;
		uint16_t maxBlockSize;
		bool debugOutput;
		char* telnetBuf;
		uint16_t bufLen;
		uint16_t bufUsed;
		uint16_t bufRdIdx;
		uint16_t bufWrIdx;
		char* recBuf;
		uint16_t recLen;
		uint16_t recUsed;
		uint16_t recRdIdx;
		uint16_t recWrIdx;
		bool connected;
		void (*callbackConnect)();
		void (*callbackDisconnect)();
        void (*callbackNvtBRK)();
        void (*callbackNvtIP)();
        void (*callbackNvtAO)();
        void (*callbackNvtAYT)();
        void (*callbackNvtEC)();
        void (*callbackNvtEL)();
        void (*callbackNvtGA)();
        void (*callbackNvtWWDD)(char command, char option);
};

#endif

