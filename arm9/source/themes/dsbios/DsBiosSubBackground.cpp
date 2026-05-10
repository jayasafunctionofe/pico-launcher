#include "common.h"
#include <string.h>
#include <nds/arm9/background.h>
#include <nds/arm9/cache.h>
#include <nds/system.h>
#include "../ITheme.h"
#include "DsBiosSubBackground.h"

#include "DsFont.h"
#include "DsIcons.h"

#include "../../rtcIpc.h"


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

// ########## Time Converstion ########## //

static int BcdToInt(u8 value)
{
    return ((value >> 4) * 10) + (value & 0x0F);
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

    _prevSecond = -1;

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

}

// ########## UPDATE ##########
void DsBiosSubBackground::Update()
{
    _frameCounter++;

    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);

    _tm.tm_sec  = BcdToInt(dateTime.time.second);
    _tm.tm_min  = BcdToInt(dateTime.time.minute);
    _tm.tm_hour = BcdToInt(dateTime.time.hour);

    _tm.tm_mday = BcdToInt(dateTime.date.monthDay);
    _tm.tm_mon  = BcdToInt(dateTime.date.month) - 1;

    int year = BcdToInt(dateTime.date.year);
    _tm.tm_year = year + 100; // 2000-based RTC year -> tm years since 1900

    _tm.tm_isdst = -1;
    
    mktime(&_tm); 
}

// ########## DRAW ##########
void DsBiosSubBackground::Draw(GraphicsContext& graphicsContext)
{
    const int sec   = _tm.tm_sec;
    const int min   = _tm.tm_min;
    const int hrs   = _tm.tm_hour;
    const int day   = _tm.tm_mday;
    const int month = _tm.tm_mon + 1;
    const int year  = _tm.tm_year + 1900;

    const bool colonVisible = ((_frameCounter / 30) & 1) == 0;

    const bool secondChanged = (sec != _prevSecond);
    const bool colonChanged  = (colonVisible != _prevColonVisible);

    if (secondChanged || colonChanged)
    {
        RestoreBgRegion(143, 0, 113, 16);
        DrawTopBarBackground();
        DrawTopBarDividers();
        DrawTopBarUserName(_systemSettings.userName);
        DrawDigitalClock(hrs, min);
        DrawTopBarDate(month, day);
        DrawTopBarGbaIcon(_systemSettings.gbaScreen);
        DrawTopBarAutoMode(_systemSettings.autoMode);
    }

    _prevSecond = sec;
    _prevColonVisible = colonVisible;
}



// ########## Draw Methods ########## //

void DsBiosSubBackground::DrawIndexedIcon(
    int x, int y,
    const u8* icon,
    int w, int h,
    const u16 palette[4])
{
    for (int py = 0; py < h; py++)
    {
        for (int px = 0; px < w; px++)
        {
            const u8 index = icon[py * w + px];

            if (index != 0)
                DrawBmpPixel(x + px, y + py, palette[index]);
        }
    }
}

void DsBiosSubBackground::DrawBmpPixel(int px, int py, u16 color)
{
    if (px < 0 || px >= 256 || py < 0 || py >= 192)
        return;
    BmpVram()[py * 256 + px] = color;
}

static int GetTextWidth(const char* text)
{
    int width = 0;

    while (*text)
    {
        const Glyph* glyph = GetGlyph(*text);
        width += glyph->width;

        text++;

        if (*text)
            width += 1; // 1px gap between chars
    }

    return width;
}

static void DrawChar(
    int originX, int originY, char ch,
    u16 color, DsBiosSubBackground* self)
{
    const Glyph* glyph = GetGlyph(ch);

    for (int row = 0; row < 8; row++)
    {
        const u8 bits = glyph->rows[row];

        for (int col = 0; col < glyph->width; col++)
        {
            if (bits & (0x80 >> col))
            {
                self->DrawBmpPixel(
                    originX + col,
                    originY + row,
                    color);
            }
        }
    }
}

static void DrawCharBig(
    int originX, int originY, char ch,
    u16 color, DsBiosSubBackground* self)
{
    const GlyphBig* glyph = GetGlyphBig(ch);

    for (int row = 0; row < 12; row++)
    {
        const u8 bits = glyph->rows[row];

        for (int col = 0; col < glyph->width; col++)
        {
            if (bits & (0x80 >> col))
            {
                self->DrawBmpPixel(
                    originX + col,
                    originY + row,
                    color);
            }
        }
    }
}

static void DrawText(
    int x, int y, const char* text,
    u16 color, DsBiosSubBackground* self)
{
    int cursorX = x;

    while (*text)
    {
        const Glyph* glyph = GetGlyph(*text);

        DrawChar(cursorX, y, *text, color, self);

        cursorX += glyph->width;
        text++;

        if (*text)
            cursorX += 1; // 1px gap
    }
}

static void DrawTextBig(
    int x, int y, const char* text,
    u16 color, DsBiosSubBackground* self)
{
    int cursorX = x;

    while (*text)
    {
        const GlyphBig* glyph = GetGlyphBig(*text);

        DrawCharBig(cursorX, y, *text, color, self);

        cursorX += glyph->width;
        text++;

        if (*text)
            cursorX += 4; // 4px gap
    }
}



// ########## TOP  BAR ##########

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

void DsBiosSubBackground::DrawTopBarUserName(const std::string& userName)
{
    constexpr int x = TB_USER_X_POS;
    constexpr int y = TB_TEXT_Y_POS;

    u16 color = _palette.white;

    DrawText(x, y, userName.c_str(), color, this);
}

void DsBiosSubBackground::DrawDigitalClock(int hour, int minute)
{
    constexpr int x = TB_CLOCK_X_POS;
    constexpr int y = TB_TEXT_Y_POS;

    u16 color = _palette.white;

    const bool showColon = ((_frameCounter / 30) & 1) == 0;

    char text[6];

    snprintf(
        text,
        sizeof(text),
        "%02d%c%02d",
        hour,
        showColon ? ':' : ' ',
        minute);

    DrawText(x, y, text, color, this);
}

void DsBiosSubBackground::DrawTopBarDate(int month, int day)
{
    constexpr int x = TB_DATE_X_POS;
    constexpr int y = TB_TEXT_Y_POS;

    u16 color = _palette.white;

    char text[6];

    snprintf(
        text,
        sizeof(text),
        "%02d/%02d",
        month,
        day);

    DrawText(x, y, text, color, this);
}

void DsBiosSubBackground::DrawTopBarGbaIcon(bool gbaScreen)
{
    constexpr int x = TB_GBA_X_POS;
    constexpr int y = TB_GBA_Y_POS;

    const u16 PALETTE_GBA[4] =
    {
        0,
        _palette.black,
        _palette.lightGray,
        _palette.mediumGray,
    };

    const u16 PALETTE_GBA_SCREEN[4] =
    {
        0,
        _palette.gbaOrange,

    };

    DrawIndexedIcon(
        x, y,
        sGbaIcon,
        GBA_ICON_W, GBA_ICON_H,
        PALETTE_GBA);
    if (gbaScreen) // if true, bottom screen
    {
        DrawIndexedIcon(x, y, sGbaOverlayBottom, GBA_ICON_W, GBA_ICON_H, PALETTE_GBA_SCREEN);
    }
    else
    {
        DrawIndexedIcon(x, y, sGbaOverlayTop, GBA_ICON_W, GBA_ICON_H, PALETTE_GBA_SCREEN);
    }
}

void DsBiosSubBackground::DrawTopBarAutoMode(bool autoMode)
{
    constexpr int x = TB_MODE_X_POS;
    constexpr int y = TB_MODE_Y_POS;

    const u8* modeIcon = autoMode ? sModeAutoIcon : sModeManualIcon;

    const u16 PALETTE_MODE[4] =
    {
        0,
        _palette.black,
        _palette.darkGray,
        _palette.white,
    };

    DrawIndexedIcon(
        x, y,
        modeIcon,
        MODE_ICON_W, MODE_ICON_H,
        PALETTE_MODE);
}