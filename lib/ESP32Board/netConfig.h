#ifndef netConfig_H
#define netConfig_H

#include "wifiConfig.h"

class NetConfig
{
public:
    NetConfig(void);
    virtual ~NetConfig();

    static const unsigned int wifiCnt = 3;
    WiFiConfig wifi[wifiCnt];

    String mDNS_name;

    long gmtOffset;      // seconds
    int  daylightOffset; // seconds
    String ntpServer;

    void Initialize(void);
};

#endif
