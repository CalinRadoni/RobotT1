#include "wifiConfig.h"

WiFiConfig::WiFiConfig(void)
    : useDHCP(true)
    , address(0u)
    , mask(0u)
    , gateway(0u)
    , srvDNS1(0u)
    , srvDNS2(0u)
{
    //
}

WiFiConfig::~WiFiConfig()
{
    //
}

void WiFiConfig::Initialize(void)
{
    SSID.clear();
    Pass.clear();

    useDHCP = true;

    address = (uint32_t)0;
    mask    = (uint32_t)0;
    gateway = (uint32_t)0;
    srvDNS1 = (uint32_t)0;
    srvDNS2 = (uint32_t)0;
}
