#include "wifiConfig.h"

WiFiConfig::WiFiConfig(void)
    : useDHCP(true)
    , address(0u)
    , mask(0u)
    , gateway(0u)
    , dns1(0u)
    , dns2(0u)
{
    memset(SSID, 0, 33);
    memset(Pass, 0, 65);
}

WiFiConfig::~WiFiConfig()
{
    //
}

void WiFiConfig::Initialize(void)
{
    memset(SSID, 0, 33);
    memset(Pass, 0, 65);

    useDHCP = true;

    address = (uint32_t)0;
    mask    = (uint32_t)0;
    gateway = (uint32_t)0;
    dns1    = (uint32_t)0;
    dns2    = (uint32_t)0;
}

