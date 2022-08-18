#include "board.h"

#include <Wire.h>
#include <Update.h>
#include <esp_ota_ops.h>

Board::Board(void)
    : SDA_pin(-1)
    , SCL_pin(-1)
{
    boardConfig.Initialize();
};

Board::~Board()
{
    //
}

unsigned int Board::Initialize(void)
{
    if (!Init_level0()) { return 0; }

    // Init NVS here

    // Load configuration
    boardConfig.LoadFromNVS();

    // for (unsigned short i = 0; i < credCnt && i < maxWiFiConfigs; ++i) {
    //     strncpy(boardConfig.wCfg[i].SSID, SSID[i], 32);
    //     strncpy(boardConfig.wCfg[i].Pass, PASS[i], 64);
    //     boardConfig.wCfg[i].SSID[32] = 0;
    //     boardConfig.wCfg[i].Pass[64] = 0;
    // }

    // boardConfig.chatID = chatID;
    // boardConfig.botName = botName;
    // boardConfig.botToken = botToken;

    for (unsigned short i = 0; i < maxWiFiConfigs && i < CredentialCount; ++i) {
        strncpy(simpleWiFi.credentials[i].SSID, boardConfig.wCfg[i].SSID, 32);
        simpleWiFi.credentials[i].SSID[32] = 0;
        strncpy(simpleWiFi.credentials[i].PASS, boardConfig.wCfg[i].Pass, 64);
        simpleWiFi.credentials[i].PASS[64] = 0;
    }

    if (!Init_level1()) { return 1; }

    simpleWiFi.Reconnect(true);

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
