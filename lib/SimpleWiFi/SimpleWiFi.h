#ifndef SimpleWiFi_H
#define SimpleWiFi_H

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#elif defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#else
#error "This code is only for ESP32 or ESP8266 with Arduino framework !"
#endif

const unsigned short CredentialCount = 3;
const unsigned long ConnectionInitWaitTime = 5000; // 5 seconds

class WiFiData {
public:
    char SSID[33];
    char PASS[65];
};

class SimpleWiFi {
public:
    SimpleWiFi(void);
    virtual ~SimpleWiFi();

    WiFiData credentials[CredentialCount];

    unsigned long connectionInitWaitTime;

    void Initialize(void);
    bool InitWiFiConnection(void);
    bool CheckConnection(bool restartOnFail);
    bool Reconnect(bool restartOnFail);
    bool IsConnected(void);

#if defined(ARDUINO_ARCH_ESP32)
    void evWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    void evWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
    void evWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
#elif defined(ARDUINO_ARCH_ESP8266)
    void evWifiConnect(const WiFiEventStationModeGotIP& event);
    void evWifiDisconnect(const WiFiEventStationModeDisconnected& event);
#endif

protected:
    bool initialized;
    bool connected;

#if defined(ARDUINO_ARCH_ESP32)
    WiFiEventId_t evidSC;
    WiFiEventId_t evidGI;
    WiFiEventId_t evidSD;
#endif
#if defined(ARDUINO_ARCH_ESP8266)
    WiFiEventHandler wifiConnectHandler;
    WiFiEventHandler wifiDisconnectHandler;
#endif
};

#endif
