#include <Arduino.h>
#include <Wire.h>

#include "SimpleWiFi.h"
#include "UpdateFromWeb.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include "credentials.h"
#include "ESPCamera.h"

#include "HIHSensor.h"

SimpleWiFi simpleWiFi;
UpdateFromWeb webUpdater;
ESPCamera espCam;
HIHSensor thSensor;

WiFiClientSecure securedClient;
UniversalTelegramBot bot(botToken, securedClient);

const long gmtOffset = 7200;        // seconds
const int daylightOffset = 3600;    // seconds

const unsigned char sBuffSize = 120;
char sbuffer[sBuffSize];

unsigned long timeOfLastMessage;
unsigned long timeRestartAfter = 2 * 86400000UL; // 48 hours

uint32_t restartRequired = 0;

int prevMinute = -1;
bool minuteChanged = false;
bool timeForTelegram = false;

/*
                            5V          3V3
                           Gnd          GPIO16 / U2RxD
            HS2_Data2 / GPIO12          GPIO0  / CSI_MCLK / ~Programming_mode
            HS2_Data3 / GPIO13          Gnd
SCL Sensor /  HS2_Cmd / GPIO15          3V3 / 5V
SDA Sensor /  HS2_Clk / GPIO14          GPIO3  / U0RxD
            HS2_Data0 / GPIO2           GPIO1  / U0TxD
            HS2_Data1 / GPIO4           Gnd
                              ESP32-CAM
                              AI-Thinker

    GPIO - 10k pull-down
    HS2_Cmd   - 47k pull-up
    HS2_Data0 - 47k pull-up
    HS2_Data1 - 47k pull-up
    HS2_Data2 - 47k pull-up
    HS2_Data3 - 47k pull-up

    LED = ~GPIO33
    LED_Flash = HS2_Data1 / GPIO4 (1k ser + 10k Gnd + Q1)
*/

const int pinLED = 33;
const int pinFlashLED = 4;
const int pinSDA = 14;
const int pinSCL = 15;

//const int pinPIR = 4;
volatile unsigned int motionDetected = 0;
unsigned int motionCount = 0;

uint16_t hihT = 0;
uint16_t hihH = 0;
bool thDataWIP = false;

portMUX_TYPE muxM = portMUX_INITIALIZER_UNLOCKED;

const char *helpMessage =
    "/ping\n" \
    "/data\n" \
    "/photo\n" \
    "/photof\n" \
    "/reset\n" \
    "/update URL";

void ResetBoard(uint32_t delayMS)
{
    log_i("Resetting the board");

    espCam.Deinit();

    if (delayMS > 0) {
        delay(delayMS);
    }
    ESP.restart();
}

void SendPeriodicTelegramMessage() {
    snprintf(sbuffer, sBuffSize, "%.1f degC, %.1f%%, %d m", (float)hihT / 10, (float)hihH / 10, motionCount);
    bot.sendMessage(chatID, sbuffer, "");
}

void SendMotionTelegramMessage() {
    snprintf(sbuffer, sBuffSize, "Motion detected, count = %d", motionCount);
    bot.sendMessage(chatID, sbuffer, "");
}

void IRAM_ATTR handlerPIR() {
    portENTER_CRITICAL_SAFE(&muxM);
    ++motionDetected;
    portEXIT_CRITICAL_SAFE(&muxM);
}

bool sendJPEGToTelegram(camera_fb_t *fb, const String& chat_id) {
    if (fb == NULL) return false;

    const String fileName = "esp32cam.jpg";
    const String boundary = "imgBoundaryRobotT1";
    const String userAgent = "RobotT1/1.0";
    const String telegramHost = "api.telegram.org";
    const uint16_t telegramPort = 443;

    if (!securedClient.connected()) {
        securedClient.connect(telegramHost.c_str(), telegramPort);
        if (!securedClient.connected()) {
            log_e("Failed to connect");
            return false;
        }
    }

    String requestHead;
    String requestTail;

    requestHead  = "--" + boundary + "\r\n";
    requestHead += "Content-Disposition: form-data; name=\"chat_id\";\r\n\r\n";
    requestHead += chat_id;
    requestHead += "\r\n";
    requestHead += "--" + boundary + "\r\n";
    requestHead += "Content-Disposition: form-data; name=\"photo\"; filename=\"" + fileName + "\"\r\n";
    requestHead += "Content-Type: image/jpeg\r\n\r\n";

    requestTail = "\r\n--" + boundary + "--\r\n";

    securedClient.println("POST /bot" + bot.getToken() + "/sendPhoto  HTTP/1.1");
    securedClient.println("Host: " + telegramHost);
    securedClient.println("User-Agent: " + userAgent);
    securedClient.println("Accept: */*");

    size_t contentLength = fb->len + requestHead.length() + requestTail.length();

    securedClient.println("Content-Length: " + String(contentLength));
    securedClient.println("Content-Type: multipart/form-data; boundary=" + boundary);
    securedClient.println();
    securedClient.print(requestHead);

    uint8_t *imgBuffer = fb->buf;
    size_t imgLen = fb->len;
    size_t writeLen, written;

    bool res = securedClient.connected();

    while(res && (imgLen > 0)) {
        writeLen = imgLen > 1024 ? 1024 : imgLen;
        written = securedClient.write(imgBuffer, writeLen);
        if (written != writeLen) {
            res = false;
        }
        imgLen -= writeLen;
        imgBuffer += writeLen;
    }

    if (securedClient.connected()) {
        // current implementation closes the connection on error
        // if the connection is closed these are useless

        securedClient.print(requestTail);

        String body, headers;
        bot.readHTTPAnswer(body, headers);

        securedClient.stop();

        body = body.substring(0, 10);
        if (body != "{\"ok\":true") {
            res = false;
        }
    }

    return res;
}

void sendPhoto(const String& chat_id, bool useFlashLED)
{
    if (!espCam.IsInitialized()) {
        if (!espCam.Initialize(psramFound())) {
            log_e("Camera initialize failed");
            bot.sendMessage(chat_id, "Camera initialize failed", "");
            espCam.Deinit();
            return;
        }
    }

    // Note: useFlashLED is useless if grab_mode != CAMERA_GRAB_LATEST
    if (useFlashLED) { digitalWrite(pinFlashLED, HIGH); }

    camera_fb_t *pic = espCam.GetImage_wait();

    if (useFlashLED) { digitalWrite(pinFlashLED, LOW); }

    if (pic == NULL) {
        log_e("Camera capture failed");
        bot.sendMessage(chat_id, "Camera capture failed", "");
    }
    else {
        if(!sendJPEGToTelegram(pic, chat_id)) {
            log_e("JPEG send error");
        }

        espCam.ReleaseImage(pic);
    }

    espCam.Deinit();
}

void handleMessage(int idx)
{
    String text = bot.messages[idx].text;
    String msg;
    String userCmd, userData;
    String chatID;

    text.trim();
    int index = text.indexOf(' ');
    if (index < 0) {
        userCmd = text;
        userData = "";
    }
    else {
        userCmd = text.substring(0, index);
        userData = text.substring(index + 1);
    }

    chatID = bot.messages[idx].chat_id;

    if (userCmd == "/help") {
        bot.sendMessage(chatID, helpMessage, "");
        return;
    }

    if (userCmd == "/ping") {
        msg = "pong " + bot.messages[idx].from_name;
        bot.sendMessage(chatID, msg, "");
        return;
    }

    if (userCmd == "/data") {
        SendPeriodicTelegramMessage();
        return;
    }

    if (userCmd == "/photo") {
        sendPhoto(chatID, false);
        return;
    }
    if (userCmd == "/photof") {
        sendPhoto(chatID, true);
        return;
    }

    if (userCmd == "/reset") {
        bot.sendMessage(chatID, "restarting...", "");
        restartRequired = 3000;
        return;
    }

    if (userCmd == "/update") {
        bot.sendMessage(chatID, "Updating ... ", "");
        if (webUpdater.UpdateFromLink(userData)) {
            bot.sendMessage(chatID, "Update OK", "");
            bot.sendMessage(chatID, "restarting...", "");
            log_i("Update OK");
            restartRequired = 3000;
        }
        else {
            bot.sendMessage(chatID, "Update failed !", "");
            log_i("Update failed !");
        }
        return;
    }

    bot.sendMessage(chatID, "42", "");
}

void handleMessages(int msgCnt)
{
    for (int i = 0; i < msgCnt; ++i) {
        String inputChatID = bot.messages[i].chat_id;
        if (inputChatID == chatID) {
            timeOfLastMessage = millis();
            handleMessage(i);
        }
    }
}

void printLocalTime()
{
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        log_e("Failed to obtain time");
        restartRequired = 1000;
        return;
    }

    log_i("%d.%02d.%02.d %02d:%02d:%02d",
        1900 + timeInfo.tm_year,
        1 + timeInfo.tm_mon,
        timeInfo.tm_mday,
        timeInfo.tm_hour,
        timeInfo.tm_min,
        timeInfo.tm_sec);
}

void CheckTimes(void)
{
    struct tm timeInfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeInfo);

    if (prevMinute < 0) {
        // prevMinute was not set, this should be the first call
        prevMinute = timeInfo.tm_min;
        return;
    }

    if (prevMinute == timeInfo.tm_min) {
        // minute not changed
        return;
    }
    prevMinute = timeInfo.tm_min;
    minuteChanged = true;

    if (prevMinute == 0) {
        // if ((timeInfo.tm_hour % 4) == 0) {
        //     // every 4 hours send a Telegram message
        //     timeForTelegram = true;
        // }
        timeForTelegram = true;
    }
}

void setup()
{
    Serial.begin(115200);

    unsigned short cnt = credCnt;
    for (unsigned short i = 0; i < cnt && i < CredentialCount; ++i) {
        strncpy(simpleWiFi.credentials[i].SSID, SSID[i], 32); simpleWiFi.credentials[i].SSID[32] = 0;
        strncpy(simpleWiFi.credentials[i].PASS, PASS[i], 64); simpleWiFi.credentials[i].PASS[64] = 0;
    }

    simpleWiFi.Reconnect(true);

    // pinMode(pinPIR, INPUT);
    // attachInterrupt(digitalPinToInterrupt(pinPIR), handlerPIR, RISING);

    pinMode(pinLED, OUTPUT);
    digitalWrite(pinLED, HIGH);

    pinMode(pinFlashLED, OUTPUT);
    digitalWrite(pinFlashLED, LOW);

    bool usePSRAM = psramFound();
    if (!espCam.Initialize(usePSRAM)) {
        log_e("Camera initialization failed !");
        espCam.Deinit();
        if (!espCam.Initialize(usePSRAM)) {
            log_e("Camera reinitialization failed !");
        }
    }

    Wire.setPins(pinSDA, pinSCL); // call Wire.begin() after this call or call it like Wire.begin(pinSDA, pinSCL)
    Wire.begin();

    thDataWIP = thSensor.ReadInit();

    while (!simpleWiFi.IsConnected()) { delay(10); }

    configTime(gmtOffset, daylightOffset, "pool.ntp.org");
    printLocalTime();

    webUpdater.PrintApplicationDescription();

    securedClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    bot.sendMessage(chatID, "I am alive !", "");

    if (espCam.IsInitialized()) {
        espCam.PrintCameraInfo();
        espCam.PrintGains(millis());
    }
    else {
        bot.sendMessage(chatID, "Camera initialization failed !", "");
    }

    espCam.KeepPowerDownOnDeepSleep();
    espCam.Deinit();

    restartRequired = 0;
    timeOfLastMessage = millis();
}

void loop()
{
    if ((millis() - timeOfLastMessage) >= timeRestartAfter) {
        restartRequired = 1000;
    }

    simpleWiFi.CheckConnection(true);

    if (motionDetected > 0) {
        portENTER_CRITICAL_SAFE(&muxM);
            motionCount += motionDetected;
            motionDetected = 0;
        portEXIT_CRITICAL_SAFE(&muxM);

        SendMotionTelegramMessage();

        log_i("Motion count: %.d", motionCount);
    }

    if (thDataWIP) {
        switch (thSensor.ReadData()) {
            case HIHSensor::Status::DATA_OK:
                portENTER_CRITICAL_ISR(&muxM);
                hihT = thSensor.GetTemperature() - 2731;
                hihH = thSensor.GetHumidity();
                thDataWIP = false;
                portEXIT_CRITICAL_SAFE(&muxM);

                log_i("Temperature %.1f degC, Humidity %.1f%%", (float)hihT / 10, (float)hihH / 10);
                break;

            case HIHSensor::Status::DATA_Err:
                thDataWIP = false;
                break;

            default: break;
        }
    }

    CheckTimes();
    if (minuteChanged){
        minuteChanged = false;
        if (thSensor.ReadInit()) {
            thDataWIP = true;
        }
    }
    if (timeForTelegram) {
        timeForTelegram = false;
        SendPeriodicTelegramMessage();
    }

    int msgCnt = bot.getUpdates(bot.last_message_received + 1);
    while (msgCnt > 0) {
        handleMessages(msgCnt);
        msgCnt = bot.getUpdates(bot.last_message_received + 1);
    }

    // reseting the board when handling Telegram messages may screw the handling mechanism
    // reset here if requested
    if (restartRequired > 0) {
        ResetBoard(restartRequired);
    }

    yield();
}
