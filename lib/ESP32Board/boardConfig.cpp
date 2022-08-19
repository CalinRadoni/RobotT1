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
    net.Initialize();

    chatID.clear();
    botName.clear();
    botToken.clear();
}

bool BoardConfig::LoadFromNVS(void)
{
    return false;
}
