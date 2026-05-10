#pragma once

#include "common.h"
#include <string>

class DsBiosSystemInfo
{
public:

    struct Settings
    {
        std::string userName = "user";
        bool autoMode = false;
        u8 themeId = 0;
        bool gbaScreen = false;
        u8 alarmHour = 12;
        u8 alarmMinute = 0;
    };

    Settings ReadSettings() const;

private:
    static std::string ReadUserName();
    static bool ReadAutoMode();
    static u8 ReadThemeId();
    static bool ReadGbaScreen();
    static u8 ReadAlarmHour();
    static u8 ReadAlarmMinute();
};