#include "DsBiosSystemInfo.h"
#include <nds/system.h>

DsBiosSystemInfo::Settings DsBiosSystemInfo::ReadSettings() const
{
    Settings settings;

    settings.userName = ReadUserName();
    settings.autoMode = ReadAutoMode();
    settings.gbaScreen = ReadGbaScreen();
    settings.themeId = ReadThemeId();
    settings.alarmHour = ReadAlarmHour();
    settings.alarmMinute = ReadAlarmMinute();

    return settings;
}

std::string DsBiosSystemInfo::ReadUserName()
{
    const PERSONAL_DATA* personal =
        reinterpret_cast<const PERSONAL_DATA*>(PersonalData);

    if (!personal)
        return "user";

    char buffer[11];
    int outPos = 0;

    for (int i = 0; i < personal->nameLen && i < 10; i++)
    {
        const u16 ch = personal->name[i];

        if (ch >= 32 && ch <= 126)
            buffer[outPos++] = static_cast<char>(ch);
        else
            buffer[outPos++] = '?';
    }

    buffer[outPos] = '\0';

    if (outPos == 0)
        return "user";

    return std::string(buffer);
}

bool DsBiosSystemInfo::ReadAutoMode()
{
    const PERSONAL_DATA* personal =
        reinterpret_cast<const PERSONAL_DATA*>(PersonalData);

    if (!personal)
        return false;

    return personal->autoMode != 0;
}

u8 DsBiosSystemInfo::ReadThemeId()
{
        const PERSONAL_DATA* personal =
        reinterpret_cast<const PERSONAL_DATA*>(PersonalData);

    if (!personal)
        return 0;

    return personal->theme;
}

bool DsBiosSystemInfo::ReadGbaScreen()
{
    const PERSONAL_DATA* personal =
        reinterpret_cast<const PERSONAL_DATA*>(PersonalData);

    if (!personal)
        return false;

    return personal->gbaScreen != 0;
}

u8 DsBiosSystemInfo::ReadAlarmHour()
{
    const PERSONAL_DATA* personal =
    reinterpret_cast<const PERSONAL_DATA*>(PersonalData);

    if (!personal)
        return 0;

    return personal->alarmHour;
}

u8 DsBiosSystemInfo::ReadAlarmMinute()
{
    const PERSONAL_DATA* personal =
    reinterpret_cast<const PERSONAL_DATA*>(PersonalData);

    if (!personal)
        return 0;

    return personal->alarmMinute;
}