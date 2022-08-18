#include "boardConfig.h"

BoardConfig::BoardConfig(void)
{
    //
}

BoardConfig::~BoardConfig()
{
    //
}

void BoardConfig::Initialize(void)
{
    for (unsigned char i = 0; i < maxWiFiConfigs; ++i)
        wCfg[i].Initialize();

    chatID.clear();
    botName.clear();
    botToken.clear();
}

bool BoardConfig::LoadFromNVS(void)
{
    return false;
}
