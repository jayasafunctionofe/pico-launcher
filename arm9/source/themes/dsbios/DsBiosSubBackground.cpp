#include "common.h"
#include <string.h>
#include <nds/arm9/background.h>
#include <nds/arm9/cache.h>
#include <nds/system.h>
#include "../ITheme.h"
#include "DsBiosSubBackground.h"

#include "DsFont.h"
#include "DsIcons.h"




// ########## BITMAP HELPERS ##########

vu16* DsBiosSubBackground::BmpVram() const
{
    return reinterpret_cast<vu16*>(
        reinterpret_cast<u8*>(BG_GFX_SUB) + 0x8000);
}

void DsBiosSubBackground::RestoreBgRegion(int x, int y, int w, int h)
{
    if (!_bgBuffer) return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > 256 ? 256 : x + w;
    int y1 = y + h > 192 ? 192 : y + h;

    vu16* vram = BmpVram();

    for (int row = y0; row < y1; row++) {
        for (int col = x0; col < x1; col++) {
            vram[row * 256 + col] = _bgBuffer[row * 256 + col];
        }
    }
}

void DsBiosSubBackground::DrawBmpPixel(int px, int py, u16 color)
{
    if (px < 0 || px >= 256 || py < 0 || py >= 192)
        return;
    BmpVram()[py * 256 + px] = color;
}


// ########## VBLANK ##########
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


// ########## LOAD RESOURCES ##########
void DsBiosSubBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    _bgBuffer.reset(new(cache_align) u16[256 * 192]);
    memset(_bgBuffer.get(), 0, 256 * 192 * sizeof(u16));

    const auto file = std::make_unique<File>();

    if (theme.OpenThemeFile(*file, "topbg.bin"))
    {
        u32 bytesRead = 0;
        file->Read(_bgBuffer.get(), 256 * 192 * sizeof(u16), bytesRead);
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

// ########## UPDATE ##########
void DsBiosSubBackground::Update()
{

}

// ########## DRAW ##########
void DsBiosSubBackground::Draw(GraphicsContext& graphicsContext)
{
    RestoreBgRegion(143, 0, 113, 16);

    DrawTopBar();
}




// ########## TOP  BAR ##########

void DsBiosSubBackground::DrawTopBar()
{
    DrawTopBarBackground();
    DrawTopBarDividers();
}


void DsBiosSubBackground::DrawTopBarBackground()
{
    static constexpr int W = 256;

    u16 colors[5] =
    {
        COL_OPAQUE | 0x0000, // 0 black
        _palette.user.base,           // 1 base
        _palette.user.light1,        // 2
        _palette.user.light2,        // 3
        _palette.user.light3,        // 4
    };

    static constexpr u8 pattern[16][2] =
    {
        {4,4}, {3,4}, {4,3}, {3,3},
        {2,3}, {3,2}, {2,2}, {2,2},
        {2,1}, {1,2}, {1,1}, {1,1},
        {1,1}, {1,1}, {1,1}, {0,0}
    };

    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < W; x++)
        {
            DrawBmpPixel(x, y, colors[pattern[y][x & 1]]);
        }
    }
}

void DsBiosSubBackground::DrawTopBarDividers()
{
    static constexpr int xs[] = {143, 175, 207, 223, 239};

    for (int x : xs)
    {
        for (int y = 0; y < 15; y++)
        {
            // pattern: 110 (draw, draw, skip)
            if ((y % 3) != 2)
            {
                DrawBmpPixel(x, y, _palette.darkGray);
            }
        }
    }
}

