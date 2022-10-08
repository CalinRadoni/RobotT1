#include "myBoard.h"

#include "credentials.h"

MyBoard::MyBoard(void)
    : setupCamOK(false)
    , setupTHSensorOK(false)
{
    //
}

MyBoard::~MyBoard(void)
{
    //
}

bool MyBoard::Init_level0(void)
{
    pinMode(pinLED, OUTPUT);
    digitalWrite(pinLED, HIGH);

    pinMode(pinFlashLED, OUTPUT);
    digitalWrite(pinFlashLED, LOW);

    // pinMode(pinPIR, INPUT);
    // attachInterrupt(digitalPinToInterrupt(pinPIR), handlerPIR, RISING);

    SDA_pin = pinSDA;
    SCL_pin = pinSCL;

    return true;
}

bool MyBoard::Init_level1(void)
{
    // --------------------------------------------------------------------------------
    // HACK Remove these when the WEB UI is working
    // --------------------------------------------------------------------------------
    boardConfig->wifi[0].SSID = SSID[0];
    boardConfig->wifi[0].Pass = PASS[0];
    boardConfig->wifi[0].useStaticIP = false;

    boardConfig->wifi[1].SSID = SSID[1];
    boardConfig->wifi[1].Pass = PASS[1];
    boardConfig->wifi[1].useStaticIP = false;

    ((MyConfig*)boardConfig)->chatID = chatID;
    ((MyConfig*)boardConfig)->botName = botName;
    ((MyConfig*)boardConfig)->botToken = botToken;

    boardConfig->gmtOffset = 7200;
    boardConfig->daylightOffset = 3600;
    boardConfig->srvNTP = "pool.ntp.org";
    // --------------------------------------------------------------------------------
    // HACK Remove these when the WEB UI is working
    // --------------------------------------------------------------------------------

    return true;
}

bool MyBoard::Init_level2(void)
{
    bool usePSRAM = psramFound();
    if (!espCam.Initialize(usePSRAM)) {
        log_e("Camera initialization failed !");
        espCam.Deinit();
        if (!espCam.Initialize(usePSRAM)) {
            log_e("Camera reinitialization failed !");
        }
    }

    setupTHSensorOK = thSensor.IsPresent();
    if (setupTHSensorOK) {
        thSensor.ReadInit();
    }

    return true;
}

bool MyBoard::Init_level3(void)
{
    setupCamOK = espCam.IsInitialized();
    if (setupCamOK) {
        espCam.PrintCameraInfo();
        espCam.PrintGains(millis());
    }
    espCam.KeepPowerDownOnDeepSleep();
    espCam.Deinit();

    return true;
}

void MyBoard::CustomReset(void)
{
    espCam.Deinit();
}
