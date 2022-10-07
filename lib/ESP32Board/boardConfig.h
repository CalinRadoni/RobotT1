#ifndef boardConfig_H
#define boardConfig_H

#include <Arduino.h>
#include <Preferences.h>

#include "wifiConfig.h"

class BoardConfig
{
public:
    BoardConfig(void);
    virtual ~BoardConfig();

    static const unsigned int wifiCnt = 3;
    WiFiConfig wifi[wifiCnt];

    String mDNSname;

    long gmtOffset;      // seconds
    int  daylightOffset; // seconds
    String srvNTP;

    void Initialize(void);
    bool Load(void);
    bool Save(void);

    bool EmptyNamespace(void);
    bool EraseDefaultNVS(void);

    virtual void CustomInit(void);
    virtual bool CustomLoad(void);
    virtual bool CustomSave(void);

protected:
    Preferences prefs;
    bool cPutInt(const char *key, int32_t val);
    bool cPutUInt(const char *key, uint32_t val);
    bool cPutString(const char *key, String val);
};

#endif
