#include "ISoundEffectService.h"
#include "common.h"
#include "fat/File.h"
#include "services/settings/IAppSettingsService.h"
#include <../../libtwl7/include/libtwl/sound/soundChannel.h>
#include "soundIpcCommand.h"

class PcmSoundEffectService : public ISoundEffectService
{
public:
    explicit PcmSoundEffectService(
        const IAppSettingsService& appSettingsService);

    ~PcmSoundEffectService();

    void LoadThemeSoundsFromConfig() override;

    void PlayTick() override;

    void PlayHourlyChime() override;

private:
    bool LoadPcmFile(
        const TCHAR* path,
        void*& outBuffer,
        u32& outSize);

private:
    struct alignas(32) PcmSoundCmdList
    {
        u32 cmdCount;
        snd_ipc_cmd_setup_channel_t setup;
        u32 startChannels;
    };
    PcmSoundCmdList _tickCmdList alignas(32);
    PcmSoundCmdList _chimeCmdList alignas(32);

private:
    const IAppSettingsService& _appSettingsService;

    void* _tickBuffer = nullptr;
    u32 _tickSize = 0;

    void* _chimeBuffer = nullptr;
    u32 _chimeSize = 0;

    static constexpr u32 TICK_CHANNEL = 2;
    static constexpr u32 CHIME_CHANNEL = 4;

    static constexpr u32 SAMPLE_RATE = 32768;
};