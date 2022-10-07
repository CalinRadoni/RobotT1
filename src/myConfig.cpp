#include "myConfig.h"

MyConfig::MyConfig(void)
{
    //
}

MyConfig::~MyConfig()
{
    //
}

void MyConfig::CustomInit(void)
{
    chatID.clear();
    botName.clear();
    botToken.clear();
}

bool MyConfig::CustomLoad(void)
{
    chatID   = prefs.getString("chatID");
    botName  = prefs.getString("botName");
    botToken = prefs.getString("botToken");

    return true;
}

bool MyConfig::CustomSave(void)
{
    bool res = true;

    if (!cPutString("chatID", chatID)) res = false;
    if (!cPutString("botName", botName)) res = false;
    if (!cPutString("botToken", botToken)) res = false;

    return res;
}

