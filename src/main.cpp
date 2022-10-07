#include "myBoard.h"
#include "myConfig.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

MyBoard board;
MyConfig config;

WiFiClientSecure securedClient;

// config.botToken is empty now but it will be updated in setup()
UniversalTelegramBot bot(config.botToken, securedClient);

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

volatile unsigned int motionDetected = 0;
unsigned int motionCount = 0;

uint16_t hihT = 0;
uint16_t hihH = 0;

portMUX_TYPE muxM = portMUX_INITIALIZER_UNLOCKED;

const char *helpMessage =
    "/ping\n" \
    "/data\n" \
    "/photo\n" \
    "/photof\n" \
    "/reset\n" \
    "/update URL";

void SendPeriodicTelegramMessage() {
    snprintf(sbuffer, sBuffSize, "%.1f degC, %.1f%%, %d m", (float)hihT / 10, (float)hihH / 10, motionCount);
    bot.sendMessage(config.chatID, sbuffer, "");
}

void SendMotionTelegramMessage() {
    snprintf(sbuffer, sBuffSize, "Motion detected, count = %d", motionCount);
    bot.sendMessage(config.chatID, sbuffer, "");
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
    if (!board.espCam.IsInitialized()) {
        if (!board.espCam.Initialize(psramFound())) {
            log_e("Camera initialize failed");
            bot.sendMessage(chat_id, "Camera initialize failed", "");
            board.espCam.Deinit();
            return;
        }
    }

    // Note: useFlashLED is useless if grab_mode != CAMERA_GRAB_LATEST
    if (useFlashLED) { digitalWrite(pinFlashLED, HIGH); }

    camera_fb_t *pic = board.espCam.GetImage_wait();

    if (useFlashLED) { digitalWrite(pinFlashLED, LOW); }

    if (pic == NULL) {
        log_e("Camera capture failed");
        bot.sendMessage(chat_id, "Camera capture failed", "");
    }
    else {
        if(!sendJPEGToTelegram(pic, chat_id)) {
            log_e("JPEG send error");
        }

        board.espCam.ReleaseImage(pic);
    }

    board.espCam.Deinit();
}

void handleMessage(int idx)
{
    String text = bot.messages[idx].text;
    String msg;
    String userCmd, userData;
    String msgChatID;

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

    msgChatID = bot.messages[idx].chat_id;

    if (userCmd == "/help") {
        bot.sendMessage(msgChatID, helpMessage, "");
        return;
    }

    if (userCmd == "/ping") {
        msg = "pong " + bot.messages[idx].from_name;
        bot.sendMessage(msgChatID, msg, "");
        return;
    }

    if (userCmd == "/data") {
        SendPeriodicTelegramMessage();
        return;
    }

    if (userCmd == "/photo") {
        sendPhoto(msgChatID, false);
        return;
    }
    if (userCmd == "/photof") {
        sendPhoto(msgChatID, true);
        return;
    }

    if (userCmd == "/reset") {
        bot.sendMessage(msgChatID, "restarting...", "");
        restartRequired = 3000;
        return;
    }

    if (userCmd == "/update") {
        bot.sendMessage(msgChatID, "Updating ... ", "");
        if (board.UpdateFromLink(userData)) {
            bot.sendMessage(msgChatID, "Update OK", "");
            bot.sendMessage(msgChatID, "restarting...", "");
            log_i("Update OK");
            restartRequired = 3000;
        }
        else {
            bot.sendMessage(msgChatID, "Update failed !", "");
            log_i("Update failed !");
        }
        return;
    }

    bot.sendMessage(msgChatID, "42", "");
}

void handleMessages(int msgCnt)
{
    for (int i = 0; i < msgCnt; ++i) {
        String inputChatID = bot.messages[i].chat_id;
        if (inputChatID == config.chatID) {
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

    board.Initialize(&config);
    bot.updateToken(config.botToken);

    configTime(gmtOffset, daylightOffset, "pool.ntp.org");
    printLocalTime();

    securedClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    bot.sendMessage(config.chatID, "I am alive !", "");
    if (!board.setupCamOK) {
        bot.sendMessage(config.chatID, "Camera initialization failed !", "");
    }

    restartRequired = 0;
    timeOfLastMessage = millis();
}

void loop()
{
    if ((millis() - timeOfLastMessage) >= timeRestartAfter) {
        restartRequired = 1000;
    }

    board.CheckConnection(true);

    if (motionDetected > 0) {
        portENTER_CRITICAL_SAFE(&muxM);
            motionCount += motionDetected;
            motionDetected = 0;
        portEXIT_CRITICAL_SAFE(&muxM);

        SendMotionTelegramMessage();

        log_i("Motion count: %.d", motionCount);
    }

    if (board.thSensor.ReadInProgress()) {
        if (HIHSensor::Status::DATA_OK == board.thSensor.ReadData()) {
                portENTER_CRITICAL_SAFE(&muxM);
                hihT = board.thSensor.GetTemperature() - 2731;
                hihH = board.thSensor.GetHumidity();
                portEXIT_CRITICAL_SAFE(&muxM);

                log_i("Temperature %.1f degC, Humidity %.1f%%", (float)hihT / 10, (float)hihH / 10);
        }
    }

    CheckTimes();
    if (minuteChanged){
        minuteChanged = false;
        board.thSensor.ReadInit();
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
        board.ResetBoard(restartRequired);
    }

    yield();
}
