#ifndef boardConfig_H
#define boardConfig_H

#include <Arduino.h>

#include "netConfig.h"

class BoardConfig
{
public:
    BoardConfig(void);
    virtual ~BoardConfig();

    NetConfig net;

    String chatID;
    String botName;
    String botToken;

    void Initialize(void);

    bool LoadFromNVS(void);
};

#endif
