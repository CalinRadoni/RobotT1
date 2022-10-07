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
    ((MyConfig*)boardConfig)->chatID = chatID;
    ((MyConfig*)boardConfig)->botName = botName;
    ((MyConfig*)boardConfig)->botToken = botToken;
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
