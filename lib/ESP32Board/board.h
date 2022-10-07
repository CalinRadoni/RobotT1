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

    int SDA_pin;
    int SCL_pin;

    const unsigned long WiFiTimeout = 60000UL; // 60 seconds

    virtual bool Init_level0(void) = 0;
    virtual bool Init_level1(void) = 0;
    virtual bool Init_level2(void) = 0;
    virtual bool Init_level3(void) = 0;

    /**
     * @brief Initialize the board
     *
     * Workflow:
     *   call Init_level0
     *   Load configuration from NVS
     *   call Init_level1
     *   Initialize WiFi connection
     *   Print application description
     *   Initialize the Wire object
     *   call Init_level2
     *   wait for WiFi to connect or timeout
     *   call Init_level3
     *
     * @return 0 if Init_level0 returns false
     * @return 1 if Init_level1 returns false
     * @return 2 if Init_level2 returns false
     * @return 3 if Init_level3 returns false
     * @return 0xE0 if WiFi connection timed out
     * @return 0xFF on success
     */
    unsigned int Initialize(BoardConfig*);

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
    BoardConfig *boardConfig;
    SimpleWiFi simpleWiFi;
    UpdateFromWeb webUpdater;
};

#endif
