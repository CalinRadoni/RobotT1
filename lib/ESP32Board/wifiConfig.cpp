#include "wifiConfig.h"

WiFiConfig::WiFiConfig(void)
    : useStaticIP(false)
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

    useStaticIP = false;

    address = (uint32_t)0;
    mask    = (uint32_t)0;
    gateway = (uint32_t)0;
    srvDNS1 = (uint32_t)0;
    srvDNS2 = (uint32_t)0;
}
