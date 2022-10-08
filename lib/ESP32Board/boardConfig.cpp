#include "boardConfig.h"

#include "nvs_flash.h"
#include "nvs.h"

const unsigned int BoardConfig::wifiCnt;

const char* nvsNamespace = "RobotT1";

BoardConfig::BoardConfig(void)
{
    //
}

BoardConfig::~BoardConfig()
{
    //
}

bool BoardConfig::EmptyNamespace(void)
{
    if (!prefs.begin(nvsNamespace, false)) {
        log_e("Failed to open namespace [%s]", nvsNamespace);
        return false;
    }

    bool res = prefs.clear();
    if (!res) {
        log_e("Failed to empty %s namespace");
    }
    prefs.end();
    return res;
}

bool BoardConfig::EraseDefaultNVS(void)
{
    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK) {
        log_e("Failed to erase the default NVS partition [%d : %s]",
            err,
            esp_err_to_name(err));
        return false;
    }
    return true;
}

bool BoardConfig::cPutInt(const char *key, int32_t val)
{
    int32_t actualVal = prefs.getInt(key);
    if (actualVal == val) { return true; }

    size_t cnt = prefs.putInt(key, val);

    return (cnt == 0) ? false : true;
}

bool BoardConfig::cPutUInt(const char *key, uint32_t val)
{
    uint32_t actualVal = prefs.getUInt(key);
    if (actualVal == val) { return true; }

    size_t cnt = prefs.putUInt(key, val);

    return (cnt == 0) ? false : true;
}

bool BoardConfig::cPutString(const char *key, String val)
{
    String actualVal = prefs.getString(key);
    if (actualVal.equals(val)) { return true; }

    size_t cnt = prefs.putString(key, val);
    if (cnt == 0) {
        if(val.length() != 0) {
            return false;
        }
    }

    return true;
}

void BoardConfig::Initialize(void)
{
    for (unsigned int i = 0; i < wifiCnt; ++i) {
        wifi[i].Initialize();
    }

    mDNSname.clear();

    gmtOffset = 0;
    daylightOffset = 0;
    srvNTP.clear();

    CustomInit();
}

bool BoardConfig::Load(void)
{
    if (!prefs.begin(nvsNamespace, true)) {
        log_e("Failed to open namespace [%s]", nvsNamespace);
        return false;
    }

    for (unsigned int i = 0; i < wifiCnt; ++i) {
        String base = "w";
        base += i;
        String str;

        str = base + "SSID"; wifi[i].SSID = prefs.getString(str.c_str());
        str = base + "Pass"; wifi[i].Pass = prefs.getString(str.c_str());

        uint32_t flags;
        str = base + "Flags"; flags = prefs.getUInt(str.c_str());
        wifi[i].useStaticIP = (flags & 0x01);

        str = base + "addr";    wifi[i].address = prefs.getUInt(str.c_str());
        str = base + "mask";    wifi[i].mask    = prefs.getUInt(str.c_str());
        str = base + "gateway"; wifi[i].gateway = prefs.getUInt(str.c_str());
        str = base + "srvDNS1"; wifi[i].srvDNS1 = prefs.getUInt(str.c_str());
        str = base + "srvDNS2"; wifi[i].srvDNS2 = prefs.getUInt(str.c_str());
    }

    mDNSname = prefs.getString("mDNSname");

    gmtOffset = prefs.getLong("gmtOffset");
    daylightOffset = prefs.getInt("daylightOffset");
    srvNTP = prefs.getString("srvNTP");

    bool res = CustomLoad();

    prefs.end();
    return res;
}

bool BoardConfig::Save(void)
{
    if (!prefs.begin(nvsNamespace, false)) {
        log_e("Failed to open namespace [%s]", nvsNamespace);
        return false;
    }

    bool res = true;
    size_t cnt;

    for (unsigned int i = 0; i < wifiCnt; ++i) {
        String base = "w";
        base += i;
        String str;

        str = base + "SSID"; if (!cPutString(str.c_str(), wifi[i].SSID)) res = false;
        str = base + "Pass"; if (!cPutString(str.c_str(), wifi[i].Pass)) res = false;

        uint32_t flags = 0;
        if (wifi[i].useStaticIP) { flags |= 0x01; }

        str = base + "Flags";   if (!cPutUInt(str.c_str(), flags)) res = false;
        str = base + "addr";    if (!cPutUInt(str.c_str(), wifi[i].address)) res = false;
        str = base + "mask";    if (!cPutUInt(str.c_str(), wifi[i].mask))    res = false;
        str = base + "gateway"; if (!cPutUInt(str.c_str(), wifi[i].gateway)) res = false;
        str = base + "srvDNS1"; if (!cPutUInt(str.c_str(), wifi[i].srvDNS1)) res = false;
        str = base + "srvDNS2"; if (!cPutUInt(str.c_str(), wifi[i].srvDNS2)) res = false;
    }

    if (!cPutString("mDNSname", mDNSname)) res = false;

    if (!cPutInt("gmtOffset", gmtOffset)) res = false;
    if (!cPutInt("daylightOffset", daylightOffset)) res = false;
    if (!cPutString("srvNTP", srvNTP)) res = false;

    if (!CustomSave()) res = false;

    prefs.end();
    return res;
    return false;
}

void BoardConfig::CustomInit(void) { /* */ }
bool BoardConfig::CustomLoad(void) { return true; }
bool BoardConfig::CustomSave(void) { return true; }
