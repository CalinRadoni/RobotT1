#include "simpleWiFi.h"

SimpleWiFi::SimpleWiFi(void)
    : connectionInitWaitTime(ConnectionInitWaitTime)
    , config(nullptr)
    , initialized(false)
    , connected(false)
{
#if defined(ARDUINO_ARCH_ESP32)
    evidSC = 0;
    evidGI = 0;
    evidSD = 0;
#endif
}

SimpleWiFi::~SimpleWiFi()
{
    if (initialized) {
        #if defined(ARDUINO_ARCH_ESP32)
        WiFi.removeEvent(evidSC);
        WiFi.removeEvent(evidGI);
        WiFi.removeEvent(evidSD);
        #endif
    }
}

void SimpleWiFi::Initialize(void)
{
    WiFi.persistent(false);

#if defined(ARDUINO_ARCH_ESP8266)
    wifiConnectHandler    = WiFi.onStationModeGotIP(       std::bind(&SimpleWiFi::evWifiConnect,    this, std::placeholders::_1));
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&SimpleWiFi::evWifiDisconnect, this, std::placeholders::_1));
#endif

#if defined(ARDUINO_ARCH_ESP32)
    evidSC = WiFi.onEvent(std::bind(&SimpleWiFi::evWiFiStationConnected,    this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    evidGI = WiFi.onEvent(std::bind(&SimpleWiFi::evWiFiGotIP,               this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    evidSD = WiFi.onEvent(std::bind(&SimpleWiFi::evWiFiStationDisconnected, this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif

    initialized = true;
}

bool SimpleWiFi::InitWiFiConnection(void)
{
    if (!initialized) {
        Initialize();
    }

    if (connected) {
        WiFi.disconnect();
        connected = false;
    }

    if (config == nullptr) {
        log_e("Config not set !");
        return false;
    }

    for (unsigned int idx = 0; idx < config->wifiCnt; ++idx) {
        if (config->wifi[idx].SSID.length() > 0) {
            log_i("Connecting to %s", config->wifi[idx].SSID.c_str());

            if (config->wifi[idx].useStaticIP) {
                if (!WiFi.config(config->wifi[idx].address, config->wifi[idx].gateway, config->wifi[idx].mask,
                    config->wifi[idx].srvDNS1, config->wifi[idx].srvDNS2)) {
                        log_e("Failed to set static IP configuration for %s", config->wifi[idx].SSID.c_str());
                    }
            }
            else {
                WiFi.config((uint32_t)0, (uint32_t)0, (uint32_t)0);
            }

            WiFi.mode(WIFI_STA);
            WiFi.begin(config->wifi[idx].SSID.c_str(), config->wifi[idx].Pass.c_str());

            unsigned long startTime = millis();
            while ((millis() - startTime) < connectionInitWaitTime) {
                if ((WiFi.status() == WL_CONNECTED) ||
                    (WiFi.status() == WL_CONNECT_FAILED)) {
                    break;
                }
                delay(10);
            }

            if (WiFi.status() == WL_CONNECTED) { return true; }
        }
    }
    return false;
}

bool SimpleWiFi::CheckConnection(bool restartOnFail)
{
    if (connected) {
        return true;
    }

    WiFi.disconnect();
    return Reconnect(restartOnFail);
}

bool SimpleWiFi::Reconnect(bool restartOnFail)
{
    if (InitWiFiConnection()) {
        log_n("Connected to WiFi AP");
        return true;
    }

    log_w("Failed to connect");
    if (restartOnFail) {
        log_w("... will restart in 10 seconds.");
        delay(10000);
        ESP.restart();
    }
    return false;
}

bool SimpleWiFi::IsConnected(void)
{
    return connected;
}

#if defined(ARDUINO_ARCH_ESP32)
void SimpleWiFi::evWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    log_n("Connected to WiFi AP");
}

void SimpleWiFi::evWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    IPAddress ip = WiFi.localIP();
    log_n("IP address %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    log_n("RSSI %d dBm", WiFi.RSSI());
    connected = true;
}

void SimpleWiFi::evWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (!connected) return;
    connected = false;
    log_n("Disconnected from WiFi AP, reason: %d", info.wifi_sta_disconnected.reason);
}
#endif

#if defined(ARDUINO_ARCH_ESP8266)
void SimpleWiFi::evWifiConnect(const WiFiEventStationModeGotIP& event)
{
    Serial.println("Connected to WiFi AP");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    connected = true;
}

void SimpleWiFi::evWifiDisconnect(const WiFiEventStationModeDisconnected& event)
{
    if (!connected) return;
    connected = false;
    Serial.print("Disconnected from WiFi AP");
}
#endif
