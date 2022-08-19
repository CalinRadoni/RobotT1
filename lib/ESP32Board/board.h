#ifndef board_H
#define board_H

#include "boardConfig.h"
#include "simpleWiFi.h"
#include "UpdateFromWeb.h"

class Board
{
public:
    Board(void);
    virtual ~Board();

    BoardConfig boardConfig;

    int SDA_pin;
    int SCL_pin;

    virtual bool Init_level0(void) = 0;
    virtual bool Init_level1(void) = 0;
    virtual bool Init_level2(void) = 0;
    virtual bool Init_level3(void) = 0;

    /**
     * @brief Initialize the board
     *
     * Workflow:
     *   call Init_level0
     *   Initialize NVS and set `configLoaded`
     *   Load configuration from NVS
     *   call Init_level1
     *   Initialize WiFi connection
     *   Print application description
     *   Initialize the Wire object
     *   call Init_level2
     *   wait for WiFi to connect
     *   call Init_level3
     */
    unsigned int Initialize(void);

    virtual void CustomReset(void) = 0;

    /**
     * @brief Reset the board
     *
     * Workflow:
     *    call CustomReset
     *    wait delayMS
     *    reset the board
     */
    void ResetBoard(uint32_t delayMS);

    bool CheckConnection(bool restartOnFail);

    bool UpdateFromLink(String);

    void PrintApplicationDescription(void);

protected:
    SimpleWiFi simpleWiFi;
    UpdateFromWeb webUpdater;

    bool configLoaded;

};

#endif
