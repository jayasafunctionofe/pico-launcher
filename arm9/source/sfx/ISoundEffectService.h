#pragma once

class ISoundEffectService
{
public:
    virtual ~ISoundEffectService() = default;

    virtual void LoadThemeSoundsFromConfig() = 0;

    virtual void PlayTick() = 0;

    virtual void PlayHourlyChime() = 0;
};