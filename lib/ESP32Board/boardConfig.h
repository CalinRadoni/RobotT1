#ifndef boardConfig_H
#define boardConfig_H

#include <Arduino.h>

#include "wifiConfig.h"

const unsigned char maxWiFiConfigs = 3;

class BoardConfig
{
public:
    BoardConfig(void);
    virtual ~BoardConfig();

    WiFiConfig wCfg[maxWiFiConfigs];
    String chatID;
    String botName;
    String botToken;

    void Initialize(void);

    bool LoadFromNVS(void);
};

#endif
