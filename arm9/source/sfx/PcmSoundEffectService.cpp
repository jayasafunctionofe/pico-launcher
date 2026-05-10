#include "PcmSoundEffectService.h"

#include <cstring>
#include <malloc.h>

#include "common.h"
#include <nds/arm9/cache.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/timer/timer.h>
#include <libtwl/rtos/rtosIrq.h>
#include "ipcChannels.h"
// #include <libtwl/sound/soundChannel.h>

#include "core/mini-printf.h"

#include <cstdio>

PcmSoundEffectService::PcmSoundEffectService(
    const IAppSettingsService& appSettingsService)
    : _appSettingsService(appSettingsService)
{
}

PcmSoundEffectService::~PcmSoundEffectService()
{
    if (_tickBuffer)
        free(_tickBuffer);

    if (_chimeBuffer)
        free(_chimeBuffer);
}

void PcmSoundEffectService::LoadThemeSoundsFromConfig()
{
    TCHAR pathBuffer[128];
    (void)pathBuffer;

    if (_tickBuffer)
    {
        free(_tickBuffer);
        _tickBuffer = nullptr;
        _tickSize = 0;
    }

    if (_chimeBuffer)
    {
        free(_chimeBuffer);
        _chimeBuffer = nullptr;
        _chimeSize = 0;
    }

    mini_snprintf(
        pathBuffer,
        sizeof(pathBuffer),
        "/_pico/themes/%s/sounds/tick.pcm",
        _appSettingsService.GetAppSettings().theme.GetString());

    LoadPcmFile(pathBuffer, _tickBuffer, _tickSize);

    mini_snprintf(
        pathBuffer,
        sizeof(pathBuffer),
        "/_pico/themes/%s/sounds/chime.pcm",
        _appSettingsService.GetAppSettings().theme.GetString());

    LoadPcmFile(pathBuffer, _chimeBuffer, _chimeSize);
}

bool PcmSoundEffectService::LoadPcmFile(const TCHAR* path, void*& outBuffer, u32& outSize)
{
    printf("pcm open %s\n", path);

    File file;

    printf("before open\n");
    if (file.Open(path, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        printf("open failed\n");
        outBuffer = nullptr;
        outSize = 0;
        return false;
    }

    printf("before size\n");
    const u32 fileSize = file.GetSize();
    printf("size=%lu\n", fileSize);

    if (fileSize == 0)
    {
        printf("empty file\n");
        file.Close();
        outBuffer = nullptr;
        outSize = 0;
        return false;
    }

    printf("before memalign\n");
    void* buffer = memalign(32, fileSize);
    printf("buffer=%p\n", buffer);

    if (!buffer)
    {
        printf("memalign failed\n");
        file.Close();
        outBuffer = nullptr;
        outSize = 0;
        return false;
    }

    u32 bytesRead = 0;

    printf("before read\n");
    const FRESULT readResult = file.Read(buffer, fileSize, bytesRead);

    if (readResult != FR_OK || bytesRead != fileSize)
    {
        printf("read failed result=%d read=%lu expected=%lu\n",
            readResult,
            bytesRead,
            fileSize);

        free(buffer);
        file.Close();
        outBuffer = nullptr;
        outSize = 0;
        return false;
    }

    printf("before flush\n");
    DC_FlushRange(buffer, fileSize);

    printf("before close\n");
    file.Close();

    outBuffer = buffer;
    outSize = fileSize;

    printf("pcm loaded\n");
    return true;
}

void PcmSoundEffectService::PlayTick()
{
    if (!_tickBuffer || _tickSize == 0)
        return;

    constexpr u32 CHANNEL_MASK = 1 << TICK_CHANNEL;

    const u32 timer = -((33513982 + SAMPLE_RATE) / (SAMPLE_RATE * 2));

    _tickCmdList =
    {
        2,
        {
            SND_IPC_CMD_SETUP_CHANNEL,
            TICK_CHANNEL,
            _tickBuffer,
            timer,
            0,
            _tickSize >> 2,
            SOUNDCNT_VOLUME(80)
                | SOUNDCNT_PAN(64)
                | SOUNDCNT_MODE_ONCE
                | SOUNDCNT_FORMAT_PCM16
        },
        (CHANNEL_MASK << 8) | SND_IPC_CMD_START_CHANNELS
    };

    DC_FlushRange(&_tickCmdList, sizeof(_tickCmdList));
    ipc_sendFifoMessage(IPC_CHANNEL_SOUND, (u32)&_tickCmdList);
}

/*
void PcmSoundEffectService::PlayHourlyChime()
{
    if (!_chimeBuffer || _chimeSize == 0)
        return;

    constexpr u32 TEST_CHIME_CHANNEL = TICK_CHANNEL;
    constexpr u32 CHANNEL_MASK = 1 << TEST_CHIME_CHANNEL;

    const u32 timer = -((33513982 + SAMPLE_RATE) / (SAMPLE_RATE * 2));

    _chimeCmdList =
    {
        2,
        {
            SND_IPC_CMD_SETUP_CHANNEL,
            TEST_CHIME_CHANNEL,
            _chimeBuffer,
            timer,
            0,
            _chimeSize >> 2,
            SOUNDCNT_VOLUME(100)
                | SOUNDCNT_PAN(64)
                | SOUNDCNT_MODE_ONCE
                | SOUNDCNT_FORMAT_PCM16
        },
        (CHANNEL_MASK << 8) | SND_IPC_CMD_START_CHANNELS
    };

    DC_FlushRange(&_chimeCmdList, sizeof(_chimeCmdList));
    ipc_sendFifoMessage(IPC_CHANNEL_SOUND, (u32)&_chimeCmdList);
}
*/

void PcmSoundEffectService::PlayHourlyChime()
{
    if (!_chimeBuffer || _chimeSize == 0)
        return;

    constexpr u32 CHANNEL_MASK = 1 << CHIME_CHANNEL;

    const u32 timer = -((33513982 + SAMPLE_RATE) / (SAMPLE_RATE * 2));

    _chimeCmdList =
    {
        2,
        {
            SND_IPC_CMD_SETUP_CHANNEL,
            CHIME_CHANNEL,
            _chimeBuffer,
            timer,
            0,
            _chimeSize >> 2,
            SOUNDCNT_VOLUME(80)
                | SOUNDCNT_PAN(64)
                | SOUNDCNT_MODE_ONCE
                | SOUNDCNT_FORMAT_PCM16
        },
        (CHANNEL_MASK << 8) | SND_IPC_CMD_START_CHANNELS
    };

    DC_FlushRange(&_chimeCmdList, sizeof(_chimeCmdList));
    ipc_sendFifoMessage(IPC_CHANNEL_SOUND, (u32)&_chimeCmdList);
}
