#include "common.h"
#include <string.h>
#include <nds/arm9/background.h>
#include "../ITheme.h"
#include "DsBiosSubBackground.h"

void DsBiosSubBackground::VBlank()
{
    REG_DISPCNT_SUB = (REG_DISPCNT_SUB & ~0xF) | 5 | (4 << 8);
    REG_BG2CNT_SUB = BG_BMP16_256x256 | BG_PRIORITY_3 | BG_COLOR_16 | BG_MAP_BASE(2);
    REG_BG2HOFS_SUB = 0;
    REG_BG2VOFS_SUB = 0;
    REG_BG2X_SUB = 0;
    REG_BG2Y_SUB = 0;
    REG_BG2PA_SUB = 256;
    REG_BG2PB_SUB = 0;
    REG_BG2PC_SUB = 0;
    REG_BG2PD_SUB = 256;
}

void DsBiosSubBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    auto tmpBuf = std::unique_ptr<u8[]>(new(cache_align) u8[256 * 192 * 2]);
    const auto file = std::make_unique<File>();
    if (theme.OpenThemeFile(*file, "topbg.bin"))
    {
        u32 bytesRead = 0;
        file->Read(tmpBuf.get(), 256 * 192 * 2, bytesRead);
        memcpy((u8*)BG_GFX_SUB + 0x8000, tmpBuf.get(), 256 * 192 * 2);
        file->Close();
    }
    
    DC_FlushRange(_bgBuffer.get(), 256 * 192 * sizeof(u16));
    dmaCopyWords(3, _bgBuffer.get(), (void*)BmpVram(), 256 * 192 * sizeof(u16));

    DsBiosSystemInfo systemInfo;
    _systemSettings = systemInfo.ReadSettings();

    const u8 themeId = _systemSettings.themeId & 0x0F;
    if (_hasCustomSystemColor)
    {
        _palette = MakeUiPalette(BaseToUserPalette(_customSystemColor));
    }
    else
    {
        _palette = MakeUiPalette(THEME_USER_PALETTES[themeId]);
    }

    DrawTopBar();
}
