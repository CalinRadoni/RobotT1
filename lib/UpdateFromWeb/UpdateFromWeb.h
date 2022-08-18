#ifndef UpdateFromWeb_H
#define UpdateFromWeb_H

#include "Arduino.h"

/**
 *
 * @attention Correct time configuration is needed for HTTPS connections. Use something like:
 * configTime(gmtOffset, daylightOffset, "pool.ntp.org");
 * to set current time.
 */

const int MaxCertLen = 2048;

/**
 * @brief Update the firmware from a web (HTTP or HTTPS) server
 *
 */
class UpdateFromWeb
{
public:
    UpdateFromWeb(void);
    virtual ~UpdateFromWeb();

    bool UpdateFromLink(String);

private:
    bool SetCertificate(String);
    bool SetCertificateForHost(String);
    char certificate[MaxCertLen];

    void PrintMessage(String);
    void PrintMessage(char*);
};

#endif
