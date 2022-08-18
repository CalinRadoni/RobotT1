#ifndef wifiConfig_H
#define wifiConfig_H

#include <Arduino.h>

class WiFiConfig
{
public:
    WiFiConfig(void);
    virtual ~WiFiConfig();

    char SSID[33];
    char Pass[65];

    bool useDHCP;
    IPAddress address;
    IPAddress mask;
    IPAddress gateway;
    IPAddress dns1;
    IPAddress dns2;

    void Initialize(void);
};

#endif
