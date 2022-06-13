#include <Arduino.h>

#include "SimpleWiFi.h"
#include "UpdateFromWeb.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include "credentials.h"
#include "esp_camera.h"
#include "esp32cam_defs.h"

SimpleWiFi simpleWiFi;
UpdateFromWeb webUpdater;

WiFiClientSecure securedClient;
UniversalTelegramBot bot(botToken, securedClient);

const long gmtOffset = 7200;        // seconds
const int daylightOffset = 3600;    // seconds

const unsigned char sBuffSize = 120;
char sbuffer[sBuffSize];

unsigned long timeOfLastMessage;
unsigned long timeRestartAfter = 2 * 86400000UL; // 48 hours

int prevMinute = -1;
bool timeForTelegram = false;

const int pinLED = 33;
const int pinFlashLED = 4;

//const int pinPIR = 4;
volatile unsigned int motionDetected = 0;
unsigned int motionCount = 0;

portMUX_TYPE muxM = portMUX_INITIALIZER_UNLOCKED;

const char *helpMessage =
    "/ping\n" \
    "/data\n" \
    "/photo\n" \
    "/photof\n" \
    "/reset\n" \
    "/update URL";

void SendTelegramMessage() {
    snprintf(sbuffer, sBuffSize, "MC: %d", motionCount);
//    bot.sendMessage(chatID, sbuffer, "");
    Serial.println(sbuffer);
}

void IRAM_ATTR handlerPIR() {
    portENTER_CRITICAL_ISR(&muxM);
    ++motionDetected;
    portEXIT_CRITICAL_ISR(&muxM);
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
    // Note: useFlashLED is useless if grab_mode != CAMERA_GRAB_LATEST

    if (useFlashLED) {
        digitalWrite(pinFlashLED, HIGH);
    }

    camera_fb_t *pic = esp_camera_fb_get();

    if (useFlashLED) {
        digitalWrite(pinFlashLED, LOW);
    }

    if (pic == NULL) {
        log_e("Camera capture failed");
        bot.sendMessage(chat_id, "Camera capture failed", "");
        return;
    }

    if(!sendJPEGToTelegram(pic, chat_id)) {
        log_e("JPEG send error");
    }

    esp_camera_fb_return(pic);
}

void handleMessage(int idx)
{
    String text = bot.messages[idx].text;
    String msg;
    String userCmd, userData;

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

    if (userCmd == "/help") {
        bot.sendMessage(bot.messages[idx].chat_id, helpMessage, "");
        return;
    }

    if (userCmd == "/ping") {
        msg = "pong " + bot.messages[idx].from_name;
        bot.sendMessage(bot.messages[idx].chat_id, msg, "");
        return;
    }

    if (userCmd == "/data") {
        SendTelegramMessage();
        return;
    }

    if (userCmd == "/photo") {
        sendPhoto(bot.messages[idx].chat_id, false);
        return;
    }
    if (userCmd == "/photof") {
        sendPhoto(bot.messages[idx].chat_id, true);
        return;
    }

    if (userCmd == "/reset") {
        bot.sendMessage(bot.messages[idx].chat_id, "restarting...", "");
        delay(1000);
        ESP.restart();
    }

    if (userCmd == "/update") {
        if (webUpdater.UpdateFromLink(userData)) {
            bot.sendMessage(bot.messages[idx].chat_id, "Update OK", "");
            log_i("Update OK");
        }
        else {
            bot.sendMessage(bot.messages[idx].chat_id, "Update failed !", "");
            log_i("Update failed !");
        }
        return;
    }

    bot.sendMessage(bot.messages[idx].chat_id, "42", "");
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
        delay(1000);
        ESP.restart();
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

    if (prevMinute == 0) {
        // if ((timeInfo.tm_hour % 4) == 0) {
        //     // every 4 hours send a Telegram message
        //     timeForTelegram = true;
        // }
        timeForTelegram = true;
    }
}

bool InitializeCamera(void)
{
    if (ESP_OK != init_esp32_camera(psramFound())) {
        log_e("Error: Camera initialization failed !");
        return false;
    }

    sensor_t *camSensor = esp_camera_sensor_get();
    if (camSensor != NULL) {
        log_i("Camera settings: AWB %d, AWB gain %d, AGC %d, AGC gain %d, gain ceiling %d, AEC %d, AEC2 %d",
            camSensor->status.awb,
            camSensor->status.awb_gain,
            camSensor->status.agc,
            camSensor->status.agc_gain,
            camSensor->status.gainceiling,
            camSensor->status.aec,
            camSensor->status.aec2);
    }

    return true;
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

    bool cameraInitialized = InitializeCamera();

    // init any I2C after working with camera settings !

    while (!simpleWiFi.IsConnected()) { delay(10); }

    configTime(gmtOffset, daylightOffset, "pool.ntp.org");
    printLocalTime();

    webUpdater.PrintApplicationDescription();

    securedClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    bot.sendMessage(chatID, "I am alive !", "");

    if (!cameraInitialized) {
        cameraInitialized = InitializeCamera();
        if (!cameraInitialized) {
            bot.sendMessage(chatID, "Camera initialization failed !", "");
            delay(1000);
            ESP.restart();
        }
    }

    timeOfLastMessage = millis();
}

void loop()
{
    if ((millis() - timeOfLastMessage) >= timeRestartAfter) {
        ESP.restart();
    }

    simpleWiFi.CheckConnection(true);

    if (motionDetected > 0) {
        portENTER_CRITICAL_ISR(&muxM);
            motionCount += motionDetected;
            motionDetected = 0;
        portEXIT_CRITICAL_ISR(&muxM);

        SendTelegramMessage();

        snprintf(sbuffer, sBuffSize, "MC: %d", motionCount);
        Serial.println(sbuffer);
    }

    CheckTimes();
    if (timeForTelegram) {
        timeForTelegram = false;
    }

    int msgCnt = bot.getUpdates(bot.last_message_received + 1);
    while (msgCnt > 0) {
        handleMessages(msgCnt);
        msgCnt = bot.getUpdates(bot.last_message_received + 1);
    }

    yield();
}