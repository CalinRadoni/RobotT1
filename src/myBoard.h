#ifndef myBoard_H
#define myBoard_H

#include "board.h"

#include "myConfig.h"

#include "ESPCamera.h"
#include "HIHSensor.h"

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

class MyBoard : public Board
{
public:
    MyBoard(void);
    virtual ~MyBoard();

    virtual bool Init_level0(void);
    virtual bool Init_level1(void);
    virtual bool Init_level2(void);
    virtual bool Init_level3(void);

    bool setupCamOK;
    bool setupTHSensorOK;

    ESPCamera espCam;

    HIHSensor thSensor;

    virtual void CustomReset(void);
};

#endif
