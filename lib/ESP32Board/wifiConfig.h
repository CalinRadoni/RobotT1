#ifndef wifiConfig_H
#define wifiConfig_H

#include <Arduino.h>

class WiFiConfig
{
public:
    WiFiConfig(void);
    virtual ~WiFiConfig();

    String SSID;
    String Pass;

    bool useStaticIP;
    IPAddress address;
    IPAddress mask;
    IPAddress gateway;
    IPAddress srvDNS1;
    IPAddress srvDNS2;

    void Initialize(void);
};

#endif
