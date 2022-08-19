#include "board.h"

#include <Wire.h>
#include <Update.h>
#include <esp_ota_ops.h>

Board::Board(void)
    : SDA_pin(-1)
    , SCL_pin(-1)
    , configLoaded(false)
{
    simpleWiFi.config = &boardConfig.net;

    boardConfig.Initialize();
};

Board::~Board()
{
    //
}

unsigned int Board::Initialize(void)
{
    if (!Init_level0()) { return 0; }

    // TODO Init NVS
    // Init NVS here

    // Load configuration
    configLoaded = boardConfig.LoadFromNVS();
    if (!configLoaded) {
        log_w("Failed to load configuration");
    }

    // boardConfig.net....................

    // boardConfig.chatID = chatID;
    // boardConfig.botName = botName;
    // boardConfig.botToken = botToken;

    if (!Init_level1()) { return 1; }

    bool wifiConnWIP = false;
    if (configLoaded) {
         wifiConnWIP = simpleWiFi.Reconnect(false);
    }

    PrintApplicationDescription();

    Wire.setPins(SDA_pin, SCL_pin); // call Wire.begin() after this call or call it like Wire.begin(pinSDA, pinSCL)
    Wire.begin();

    if(!Init_level2()) { return 2; }

    while (!simpleWiFi.IsConnected()) {
        delay(10);
        yield();
    }

    if (!Init_level3()) { return 3; }

    return 0xFF;
}

void Board::ResetBoard(uint32_t delayMS)
{
    log_i("Resetting the board");

    CustomReset();

    if (delayMS > 0) {
        delay(delayMS);
    }
    ESP.restart();
}

bool Board::CheckConnection(bool restartOnFail)
{
    return simpleWiFi.CheckConnection(restartOnFail);
}

bool Board::UpdateFromLink(String link)
{
    return webUpdater.UpdateFromLink(link);
}

void Board::PrintApplicationDescription(void)
{
    const esp_app_desc_t *appDesc = esp_ota_get_app_description();
    if (appDesc == nullptr) {
        log_e("Failed to get application description");
        return;
    }

    log_i("%s %s", appDesc->project_name, appDesc->version);
    log_i("Compiled with ESP-IDF %s on %s %s",
        appDesc->idf_ver,
        appDesc->date,
        appDesc->time);
}
