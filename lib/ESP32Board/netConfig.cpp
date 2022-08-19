#include "netConfig.h"

const unsigned int NetConfig::wifiCnt;

NetConfig::NetConfig(void)
{
    //
}

NetConfig::~NetConfig()
{
    //
}

void NetConfig::Initialize(void)
{
    for (unsigned int i = 0; i < wifiCnt; ++i) {
        wifi[i].Initialize();
    }

    mDNS_name.clear();

    gmtOffset = 0;
    daylightOffset = 0;
    ntpServer.clear();
}
