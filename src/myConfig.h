#ifndef myConfig_H
#define myConfig_H

#include "boardConfig.h"

class MyConfig : public BoardConfig
{
public:
    MyConfig(void);
    virtual ~MyConfig();

    String chatID;
    String botName;
    String botToken;

    virtual void CustomInit(void);
    virtual bool CustomLoad(void);
    virtual bool CustomSave(void);
};

#endif
