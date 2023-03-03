/*
 * TELNET SERVER FOR ESP8266 / ESP32
 * Cloning the serial port via Telnet.
 *
 * Written by Wolfgang Mattis (arduino@wm0.eu).
 * Version 1.41 / June 26, 2022.
 * MIT license, all text above must be included in any redistribution.   
 */

#ifdef ESP8266
extern "C" {
	#include "user_interface.h"
}
#endif

#include "TelnetSpy.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

static TelnetSpy* actualObject = NULL;


static void TelnetSpy_putc(char c) {
	if (NULL != actualObject) {
  		actualObject->debugWrite(c);
	}
}

static void TelnetSpy_ignore_putc(char c) {;
}

TelnetSpy::TelnetSpy() {
	port = TELNETSPY_PORT;
	telnetServer = NULL;
	started = false;
	listening = false;
	firstMainLoop = true;
	usedSer = &Serial;
	storeOffline = true;
	connected = false;
	callbackConnect = NULL;
	callbackDisconnect = NULL;
    callbackNvtBRK = NULL;
    callbackNvtIP = (void(*)()) 1;  // 1 => use ESP.restart;
    callbackNvtAO = (void(*)()) 1;  // 1 => use TelnetSpy.disconnectClient()
    callbackNvtAYT = NULL;
    callbackNvtEC = NULL;
    callbackNvtEL = NULL;
    callbackNvtGA = NULL;
    callbackNvtWWDD = NULL;
	welcomeMsg = strdup(TELNETSPY_WELCOME_MSG);
	rejectMsg = strdup(TELNETSPY_REJECT_MSG);
    filterChar = 0;
    filterMsg = NULL;
    filterCallback = NULL;
	minBlockSize = TELNETSPY_MIN_BLOCK_SIZE;
	collectingTime = TELNETSPY_COLLECTING_TIME;
	maxBlockSize = TELNETSPY_MAX_BLOCK_SIZE;
	pingTime = TELNETSPY_PING_TIME;
	pingRef = 0xFFFFFFFF;
	waitRef = 0xFFFFFFFF;
    nvtDetected = false;
	telnetBuf = NULL;
	bufLen = 0;
	uint16_t size = TELNETSPY_BUFFER_LEN;
	while (!setBufferSize(size)) {
		size = size >> 1;
		if (size < minBlockSize) {
			setBufferSize(minBlockSize);
			break;
		}
	}
	recBuf = NULL;
	recLen = 0;
    setRecBufferSize(TELNETSPY_REC_BUFFER_LEN);
	debugOutput = TELNETSPY_CAPTURE_OS_PRINT;
	if (debugOutput) {
		setDebugOutput(true);
	}
}

TelnetSpy::~TelnetSpy() {
	end();
	if (welcomeMsg) free(welcomeMsg);
	if (rejectMsg) free(rejectMsg);
    if (filterMsg) free(filterMsg);
	if (telnetBuf) free(telnetBuf);
	if (recBuf) free(recBuf);
}

void TelnetSpy::setPort(uint16_t portToUse) {
	port = portToUse;
	if (listening) {
		if (client.connected()) {
            sendBlock();
			client.flush();
			client.stop();
		}
		if (connected && (callbackDisconnect != NULL)) {
			callbackDisconnect();
		}
		connected = false;
		telnetServer->close();
		delete telnetServer;
		telnetServer = new WiFiServer(port);
		if (started) {
			telnetServer->begin();
			telnetServer->setNoDelay(bufLen > 0);
		}
	}
}

void TelnetSpy::setWelcomeMsg(const char* msg) {
	if (welcomeMsg) {
		free(welcomeMsg);
	}
	welcomeMsg = strdup(msg);
}

void TelnetSpy::setWelcomeMsg(const String& msg) {
	if (welcomeMsg) {
		free(welcomeMsg);
	}
	welcomeMsg = strdup(msg.c_str());
}

void TelnetSpy::setRejectMsg(const char* msg) {
	if (rejectMsg) {
		free(rejectMsg);
	}
	rejectMsg = strdup(msg);
}

void TelnetSpy::setRejectMsg(const String& msg) {
	if (rejectMsg) {
		free(rejectMsg);
	}
	rejectMsg = strdup(msg.c_str());
}

void TelnetSpy::setMinBlockSize(uint16_t minSize) {
	minBlockSize = min(max((uint16_t) 1, minSize), maxBlockSize);
}
    
void TelnetSpy::setCollectingTime(uint16_t colTime) {
	collectingTime = colTime;
}

void TelnetSpy::setMaxBlockSize(uint16_t maxSize) {
	maxBlockSize = max(maxSize, minBlockSize);
}

bool TelnetSpy::setBufferSize(uint16_t newSize) {
	if (telnetBuf && (bufLen == newSize)) {
		return true;
	}
	if (newSize == 0) {
		bufLen = 0;
		if (telnetBuf) {
			free(telnetBuf);
			telnetBuf = NULL;
		}
		if (telnetServer) {
			telnetServer->setNoDelay(false);
		}
		return true;
	}
	newSize = max(newSize, minBlockSize);
	uint16_t oldBufLen = bufLen;
	bufLen = newSize;
	uint16_t tmp;
	if (!telnetBuf || (bufUsed == 0)) {
		bufRdIdx = 0;
		bufWrIdx = 0;
		bufUsed = 0;
	} else {
		if (bufLen < oldBufLen) {
			if (bufRdIdx < bufWrIdx) {
				if (bufWrIdx > bufLen) {
					tmp = min(bufLen, (uint16_t) (bufWrIdx - max(bufLen, bufRdIdx)));
					memcpy(telnetBuf, &telnetBuf[bufWrIdx - tmp], tmp);
					bufWrIdx = tmp;
					if (bufWrIdx > bufRdIdx) {
						bufRdIdx = bufWrIdx;
					} else {
						if (bufRdIdx > bufLen) {
							bufRdIdx = 0;
						}
					}
					if (bufRdIdx == bufWrIdx) {
						bufUsed = bufLen;
					} else {
						bufUsed = bufWrIdx - bufRdIdx;
					}
				}
			} else {
				if (bufWrIdx > bufLen) {
					memcpy(telnetBuf, &telnetBuf[bufWrIdx - bufLen], bufLen);
					bufRdIdx = 0;
					bufWrIdx = 0;
					bufUsed = bufLen;
				} else {
					tmp = min(bufLen - bufWrIdx, oldBufLen - bufRdIdx);
					memcpy(&telnetBuf[bufLen - tmp], &telnetBuf[oldBufLen - tmp], tmp);
					bufRdIdx = bufLen - tmp;
					bufUsed = bufWrIdx + tmp;
				}
			}
		}
	}
	char* temp = (char*) realloc(telnetBuf, bufLen);
	if (!temp) {
		return false;
	}
	telnetBuf = temp;
	if (telnetBuf && (bufLen > oldBufLen) && (bufRdIdx > bufWrIdx)) {
		tmp = bufLen - (oldBufLen - bufRdIdx);
		memcpy(&telnetBuf[tmp], &telnetBuf[bufRdIdx], oldBufLen - bufRdIdx);
		bufRdIdx = tmp;
	}
	if (telnetServer) {
		telnetServer->setNoDelay(true);
	}
	return true;
}

uint16_t TelnetSpy::getBufferSize() {
	if (!telnetBuf) {
		return 0;
	}
	return bufLen;
}

void TelnetSpy::setStoreOffline(bool store) {
	storeOffline = store;
}

bool TelnetSpy::getStoreOffline() {
	return storeOffline;
}

void TelnetSpy::setPingTime(uint16_t pngTime) {
	pingTime = pngTime;
	if (pingTime == 0) {
		pingRef = 0xFFFFFFFF;
	} else {
		pingRef = (millis() & 0x7FFFFFF) + pingTime;
	}
}

bool TelnetSpy::setRecBufferSize(uint16_t newSize) {
	if (recBuf && (recLen == newSize)) {
		return true;
	}
	if (recBuf) {
		free(recBuf);
		recBuf = NULL;
        recLen = 0;
	}
	if (newSize == 0) {
		return true;
	}
	recBuf = (char*) malloc(newSize);
	if (!recBuf) {
		return false;
    }
    recLen = newSize;
	recRdIdx = 0;
	recWrIdx = 0;
	recUsed = 0;
	return true;
}

uint16_t TelnetSpy::getRecBufferSize() {
	if (!recBuf) {
		return 0;
	}
	return recLen;
}


#if ARDUINO_USB_CDC_ON_BOOT
void TelnetSpy::setSerial(USBCDC* usedSerial) {
#else
void TelnetSpy::setSerial(HardwareSerial* usedSerial) {
#endif
	usedSer = usedSerial;
}

size_t TelnetSpy::write (uint8_t data) {
	if (telnetBuf) {
		if (storeOffline || client.connected()) {
			if (bufUsed == bufLen) {
				if (client.connected()) {
					sendBlock();
				}
				if (bufUsed == bufLen) {
					char c;
					while (bufUsed > 0) {
						c = pullTelnetBuf();
						if (c == '\n') {
							break;
						}
					}
					if (peekTelnetBuf() == '\r') {
						pullTelnetBuf();
					}
				}
			}
			addTelnetBuf(data);
		}
	} else {
		if (client.connected()) {
			client.write(data);
		}
	}
	if ((NULL != usedSer) && *usedSer) {
		return usedSer->write(data);
	}
	return 1;
}

void TelnetSpy::debugWrite (uint8_t data) {
	if (telnetBuf) {
		if (storeOffline || client.connected()) {
			if (bufUsed == bufLen) {
				char c;
				while (bufUsed > 0) {
					c = pullTelnetBuf();
					if (c == '\n') {
						break;
					}
				}
				if (peekTelnetBuf() == '\r') {
					pullTelnetBuf();
				}
			}
			addTelnetBuf(data);
		}
	}
#ifdef ESP8266
    ets_putc(data);
#else
    ets_write_char_uart(data);
#endif
}

int TelnetSpy::available (void) {
	if (usedSer) {
		int avail = usedSer->available();
		if (avail > 0) {
			return avail;
		}
	}
	if (client.connected()) {
		return telnetAvailable();
	}
	return 0;
}

int TelnetSpy::read (void) {
	int val = -1;
	if (usedSer) {
		val = usedSer->read();
		if (val != -1) {
			return val;
		}
	}
	if (client.connected()) {
		if (telnetAvailable()) {
            if (recBuf) {
                if (recUsed == 0) {
                    val = -1;
                } else {
CRITCAL_SECTION_START
                    val = recBuf[recRdIdx++];
	                if (recRdIdx >= recLen) {
		                recRdIdx = 0;
	                }
	                recUsed--;
CRITCAL_SECTION_END
                }
            } else {
			    val = client.read();
            }
		}
	}
	return val;
}

int TelnetSpy::peek (void) {
	int val = -1;
	if (usedSer) {
		val = usedSer->peek();
		if (val != -1) {
			return val;
		}
	}
	if (client.connected()) {
		if (telnetAvailable()) {
            if (recBuf) {
                val = recBuf[recRdIdx];
            } else {
			    val = client.peek();
            }
		}
	}
	return val;
}

void TelnetSpy::flush (void) {
	if (usedSer) {
		usedSer->flush();
	}
	if (client.connected()) {
        sendBlock();
        client.flush();
    }
}

#ifdef ESP8266

void TelnetSpy::begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin) {
	if (usedSer) {
		usedSer->begin(baud, config, mode, tx_pin);
	}
    setDebugOutput(debugOutput);
	started = true;
}

#else // ESP32

void TelnetSpy::begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert) {
	if (usedSer) {
#if ARDUINO_USB_CDC_ON_BOOT
		usedSer->begin(baud);
#else
		usedSer->begin(baud, config, rxPin, txPin, invert);
#endif
	}
    setDebugOutput(debugOutput);
	started = true;
}

#endif

void TelnetSpy::end() {
	if (debugOutput) {
		setDebugOutput(false);
	}
	if (usedSer) {
		usedSer->end();
	}
	if (client.connected()) {
        sendBlock();
		client.flush();
		client.stop();
	}
	if (connected && (callbackDisconnect != NULL)) {
		callbackDisconnect();
	}	
	connected = false;
	telnetServer->close();
	delete telnetServer;
	telnetServer = NULL;
	listening = false;
	started = false;
}

#ifdef ESP8266

void TelnetSpy::swap(uint8_t tx_pin) {
	if (usedSer) {
		usedSer->swap(tx_pin);
	}
}

void TelnetSpy::set_tx(uint8_t tx_pin) {
	if (usedSer) {
		usedSer->set_tx(tx_pin);
	}
}

void TelnetSpy::pins(uint8_t tx, uint8_t rx) {
	if (usedSer) {
		usedSer->pins(tx, rx);
	}
}

bool TelnetSpy::isTxEnabled(void) {
	if (usedSer) {
		return usedSer->isTxEnabled();
	}
	return true;
}

bool TelnetSpy::isRxEnabled(void) {
	if (usedSer) {
		return usedSer->isRxEnabled();
	}
	return true;
}

#endif

int TelnetSpy::availableForWrite(void) {
	if (usedSer) {
		return min(usedSer->availableForWrite(), bufLen - bufUsed);
	}
	return bufLen - bufUsed;
}

TelnetSpy::operator bool() const {
	if (usedSer) {
		return (bool) *usedSer;
	}
	return true;
}

void TelnetSpy::setDebugOutput(bool en) {
	debugOutput = en;
	if (debugOutput) {
		actualObject = this;
		ets_install_putc1(TelnetSpy_putc);  // Set system printing (os_printf) to TelnetSpy
#ifdef ESP8266
		system_set_os_print(true);
#endif
	} else {
		if (actualObject == this) {
#ifdef ESP8266
			system_set_os_print(false);
#endif
			ets_install_putc1(TelnetSpy_ignore_putc); // Ignore system printing
			actualObject = NULL;
		}
	}
}

uint32_t TelnetSpy::baudRate(void) {
	if (usedSer) {
		return usedSer->baudRate();
	}
	return 115200;
}

void TelnetSpy::sendBlock() {
CRITCAL_SECTION_START
	uint16_t len = bufUsed;
	if (len > maxBlockSize) {
		len = maxBlockSize;
	}
	len = min(len, (uint16_t) (bufLen - bufRdIdx));
	uint16_t idx = bufRdIdx;
CRITCAL_SECTION_END
    if (len == 0) {
        return;
    }
	client.write(&telnetBuf[idx], len);
CRITCAL_SECTION_START
	bufRdIdx += len;
	if (bufRdIdx >= bufLen) {
		bufRdIdx = 0;
	}
	bufUsed -= len;
	if (bufUsed == 0) {
		bufRdIdx = 0;
		bufWrIdx = 0;
	}
CRITCAL_SECTION_END
	waitRef = 0xFFFFFFFF;
	if (pingRef != 0xFFFFFFFF) {
		pingRef = (millis() & 0x7FFFFFF) + pingTime;
		if (pingRef > 0x7FFFFFFF) {
			pingRef -= 0x80000000;
		}
	}
}

void TelnetSpy::addTelnetBuf(char c) {
CRITCAL_SECTION_START
	telnetBuf[bufWrIdx] = c;
	if (bufUsed == bufLen) {
		bufRdIdx++;
		if (bufRdIdx >= bufLen) {
			bufRdIdx = 0;
		}
	} else {
		bufUsed++;
	}
	bufWrIdx++;
	if (bufWrIdx >= bufLen) {
		bufWrIdx = 0;
	}
CRITCAL_SECTION_END
}

char TelnetSpy::pullTelnetBuf() {
	if (bufUsed == 0) {
		return 0;
	}
CRITCAL_SECTION_START
	char c = telnetBuf[bufRdIdx++]; 
	if (bufRdIdx >= bufLen) {
		bufRdIdx = 0;
	}
	bufUsed--;
CRITCAL_SECTION_END
	return c;
}

char TelnetSpy::peekTelnetBuf() {
	if (bufUsed == 0) {
		return 0;
	}
CRITCAL_SECTION_START
    char c = telnetBuf[bufRdIdx];
CRITCAL_SECTION_END
    return c;
}

int TelnetSpy::telnetAvailable() {
    checkReceive();
    if (recBuf) {
        return recUsed;
    }
	return client.available();
}

bool TelnetSpy::isClientConnected() {
	return connected;
}

void TelnetSpy::setCallbackOnConnect(void (*callback)()) {
	callbackConnect = callback;
}

void TelnetSpy::setCallbackOnDisconnect(void (*callback)()) {
	callbackDisconnect = callback;
}

void TelnetSpy::disconnectClient() {
    if (client.connected()) {
        sendBlock();
        client.flush();
        client.stop();
    }
    if (connected && (callbackDisconnect != NULL)) {
        callbackDisconnect();
    }
    connected = false;
}

void TelnetSpy::clearBuffer() {
	bufUsed = 0;
	bufRdIdx = 0;
	bufWrIdx = 0;
}

void TelnetSpy::setFilter(char ch, const char* msg, void (*callback)()) {
    filterChar = ch;
    if (filterMsg) {
        free(filterMsg);
    }
    filterMsg = strdup(msg);
    filterCallback = callback;
}

void TelnetSpy::setFilter(char ch, const String& msg, void (*callback)()) {
    filterChar = ch;
    if (filterMsg) {
        free(filterMsg);
    }
    filterMsg = strdup(msg.c_str());
    filterCallback = callback;
}

char TelnetSpy::getFilter() {
    return filterChar;
}

void TelnetSpy::setCallbackOnNvtBRK(void (*callback)()) {
	callbackNvtBRK = callback;
}

void TelnetSpy::setCallbackOnNvtIP(void (*callback)()) {
	callbackNvtIP = callback;
}

void TelnetSpy::setCallbackOnNvtAO(void (*callback)()) {
	callbackNvtAO = callback;
}

void TelnetSpy::setCallbackOnNvtAYT(void (*callback)()) {
	callbackNvtAYT = callback;
}

void TelnetSpy::setCallbackOnNvtEC(void (*callback)()) {
	callbackNvtEC = callback;
}

void TelnetSpy::setCallbackOnNvtEL(void (*callback)()) {
	callbackNvtEL = callback;
}

void TelnetSpy::setCallbackOnNvtGA(void (*callback)()) {
	callbackNvtGA = callback;
}

void TelnetSpy::setCallbackOnNvtWWDD(void (*callback)(char command, char option)) {
	callbackNvtWWDD = callback;
}

void TelnetSpy::handle() {
	if (firstMainLoop) {
		firstMainLoop = false;
    	// Between setup() and loop() the configuration for os_print may be changed so it must be renewed
		if (debugOutput && (actualObject == this)) {
			setDebugOutput(true);
		}
	}
	if (!started) {
		return;
	}
	if (!listening) {
        switch (WiFi.getMode()) {
            case WIFI_MODE_STA:
                if (WiFi.status() != WL_CONNECTED) {
                    return;
                }
                break;
            case WIFI_MODE_AP:
            case WIFI_MODE_APSTA:
                break;
            default:
                return;
        }
		telnetServer = new WiFiServer(port);
		telnetServer->begin();
		telnetServer->setNoDelay(bufLen > 0);
		listening = true;
	}
    if (telnetServer->hasClient()) {
        if (client.connected()) {
            WiFiClient rejectClient = telnetServer->available();
			if (strlen(rejectMsg) > 0) {
				rejectClient.write((const uint8_t*) rejectMsg, strlen(rejectMsg));
			}
			rejectClient.flush();
            rejectClient.stop();
        } else {
            client = telnetServer->available();
			if (strlen(welcomeMsg) > 0) {
				client.write((const uint8_t*) welcomeMsg, strlen(welcomeMsg));
			}
        }
    }
    if (client.connected()) {
    	if (!connected) {
    		connected = true;
    		if (pingTime != 0) {
    			pingRef = (millis() & 0x7FFFFFF) + pingTime;
    		}
			if (callbackConnect != NULL) {
				callbackConnect();
			}
		}
	} else {
    	if (connected) {
    		connected = false;
            sendBlock();
        	client.flush();
            client.stop();
			pingRef = 0xFFFFFFFF;
			waitRef = 0xFFFFFFFF;
			if (callbackDisconnect != NULL) {
				callbackDisconnect();
			}
		}
	}

	if (client.connected() && (bufUsed > 0)) {
		if (bufUsed >= minBlockSize) {
			sendBlock();
		} else {
			unsigned long m = millis() & 0x7FFFFFF;
			if (waitRef == 0xFFFFFFFF) {
				waitRef = m + collectingTime;
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
	if (client.connected() && (pingRef != 0xFFFFFFFF)) {
		unsigned long m = millis() & 0x7FFFFFF;
		if (!((pingRef < 0x20000000) && (m > 0x60000000)) && (m >= pingRef)) {
            if (nvtDetected) {
                // Send a NOP via telnet NVT protocol
			    addTelnetBuf(255);
			    addTelnetBuf(241);
            } else  {
                // Send a NULL
			    addTelnetBuf(0);
            }
			sendBlock();
		}
	}
    if (client.connected()) {
        checkReceive();
    }
}

void TelnetSpy::writeRecBuf(char c) {
    if (recLen == recUsed) {
        return;
    }
CRITCAL_SECTION_START
	recBuf[recWrIdx++] = c;
	if (recWrIdx >= recLen) {
		recWrIdx = 0;
	}
	recUsed++;
CRITCAL_SECTION_END
}

void TelnetSpy::checkReceive() {
	int n = client.available();
	while (n > 0) {
        char c, c2;
        c = client.peek();
        if (filterChar && (filterChar == c)) {
            // Filter character detected
			if (strlen(filterMsg) > 0) {
				client.write((const uint8_t*) filterMsg, strlen(filterMsg));
			}
   			client.read();  // Remove filter character
            n--;
            if (filterCallback != NULL) {
                filterCallback();
            }
            continue;
        }
		if (255 == c) {  // If IAC (start of telnet NVT protocol telegram):
            if (n == 1) {
                // Telegram incomplete
                return;
            }
   			client.read();  // Remove IAC
            n--;
            c = client.read();  // Gett command byte
            n--;
            switch (c) {
                case 241:   // Telnet command "NOP" (no operation)
              		if (pingTime != 0) {
  		            	pingRef = (millis() & 0x7FFFFFF) + pingTime;
  		            }
                    break;
                case 242:   // Telnet command "Data Mark" (not yet implemented)
                    break;
                case 243:   // Telnet command "Break";
                    if (callbackNvtBRK != NULL) {
                        callbackNvtBRK();
                    }
                    break;
                case 244:   // Telnet command "Interrupt process"
                    if (callbackNvtIP != NULL) {
                        if ((void(*)()) 1 == callbackNvtIP) {
                            ESP.restart();
                        } else {
                            callbackNvtIP();
                        }
                    }
                    break;
                case 245:   // Telnet command "Abort output"
                    if (callbackNvtAO != NULL) {
                        if ((void(*)()) 1 == callbackNvtAO) {
                            disconnectClient();
                        } else {
                            callbackNvtAO();
                        }
                    }
                    break;
                case 246:   // Telnet command "Are you there"
                    if (callbackNvtAYT != NULL) {
                        callbackNvtAYT();
                    }
                    break;
                case 247:   // Telnet command "Erase character"
                    if (callbackNvtEC != NULL) {
                        callbackNvtEC();
                    }
                    break;
                case 248:   // Telnet command "Erase line"
                    if (callbackNvtEL != NULL) {
                        callbackNvtEL();
                    }
                    break;
                case 249:   // Telnet command "Go ahead"
                    if (callbackNvtGA != NULL) {
                        callbackNvtGA();
                    }
                    break;
                case 250:   // Telnet command "SB" (additional data follows)
                    while (n > 0) {
                        c = client.read();
                        n--;
                        if (255 != c) {
                            // If not IAC, ignore it
                            continue;
                        }
                        c = client.read();
                        n--;
                        if (240 != c) {
                            // If not SE (end of additional data), ignore it
                            continue;
                        }
                    }
                    break;
                case 251:   // Telnet command "WILL"
                case 252:   // Telnet command "WON'T"
                case 253:   // Telnet command "DO"
                case 254:   // Telnet command "DON'T"
                    nvtDetected = true;
                    c2 = client.read();     // Get option byte
                    n--;
                    if (callbackNvtWWDD != NULL) {
                        callbackNvtWWDD(c, c2);
                    }
                    break;
                case 255:   // Escaped data byte 0xff
                    if (recBuf) {
                        writeRecBuf(c);
                    } else {
                        // If no receive buffer is used, the data byte 0xff will be lost.
                        // May be in the future there is a solution for this problem.
                    }
                    break;
            }
            continue;
		}
        // Next character in the client buffer is a normal character
        if (recBuf) {
            client.read();
            writeRecBuf(c);
            n--;
            continue;
        }
        // Leave the character in the client buffer
        return;
	}
}

