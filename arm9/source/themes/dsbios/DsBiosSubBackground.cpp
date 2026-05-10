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
#include "sharedMemory.h"


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
    
    // clock back - 14, 46  101 x 102
    const auto clockFile = std::make_unique<File>();
    if (theme.OpenThemeFile(*clockFile, "dsbios_clock.bin"))
    {
        DrawBinImage(*clockFile, 13, 45, 101, 101, true);
        clockFile->Close();
    }
    // calendar back -  125, 32  117 x 115
    const auto calFile = std::make_unique<File>();
    if (theme.OpenThemeFile(*calFile, "dsbios_calendar.bin"))
    {
        DrawBinImage(*calFile, 125, 31, 117, 115, true);
        calFile->Close();
    }
    // flyout back  0,146   256 x 47
    const auto flyoutFile = std::make_unique<File>();
    if (theme.OpenThemeFile(*flyoutFile, "dsbios_flyout.bin"))
    {
        DrawBinImage(*flyoutFile, 0, 146, 256, 47, true);
        flyoutFile->Close();
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

    _prevBatteryVisible = true;
    _batteryLow = (_dsBattLevel <= 3);

    if (isDSiMode())
    {
        DrawTopBarBatteryIconDsi(SHARED_BATTERY_STATE);
    }
    else
    {
        DrawTopBarBatteryIcon(_batteryLow);
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

    // Read battery once every ~5 seconds at 60 FPS
    if ((_frameCounter % 600) == 0)
    {  
        _dsBattLevel = SHARED_BATTERY_STATE & BATTERY_LEVEL_MASK;
        _batteryLow = (_dsBattLevel == BATTERY_LEVEL_DS_LOW);
    }
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
    const bool dayChanged    = (day != _prevDay);

    const bool batteryVisible = !_batteryLow || (((_frameCounter / 30) & 1) == 0);
    const bool batteryChanged = (batteryVisible != _prevBatteryVisible);

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

        if (isDSiMode())
        {
            DrawTopBarBatteryIconDsi(SHARED_BATTERY_STATE);
        }
        else
        {
            DrawTopBarBatteryIcon(_batteryLow);
        }
    }

    if (secondChanged)
    {
        RestoreBgRegion(
            CLOCK_CX - CLOCK_RESTORE_R,
            CLOCK_CY - CLOCK_RESTORE_R,
            CLOCK_RESTORE_R * 2 + 1,
            CLOCK_RESTORE_R * 2 + 1);

        DrawClockHands(hrs, min, sec, _systemSettings.alarmHour, _systemSettings.alarmMinute);
    }

    if (dayChanged)
    {
        RestoreBgRegion(
            CAL_ORIGIN_X, CAL_ORIGIN_Y,
            CAL_CELL_W * 7,
            CAL_CELL_H * 6);

        DrawCalendar(year, month, day);
    }

    _prevDay = day;
    _prevSecond = sec;
    _prevColonVisible = colonVisible;
    _prevBatteryVisible = batteryVisible;
    
}



// ########## Draw Methods ########## //

void DsBiosSubBackground::DrawBgBufferPixel(int px, int py, u16 color)
{
    if (px < 0 || px >= 256 || py < 0 || py >= 192)
        return;

    if (!_bgBuffer)
        return;

    _bgBuffer[py * 256 + px] = color;
}

bool DsBiosSubBackground::DrawBinImage(
    File& file,
    int x,
    int y,
    int width,
    int height,
    bool useTransparency
)
{
    const u32 pixelCount = width * height;
    const u32 byteCount = pixelCount * sizeof(u16);

    auto pixels = std::unique_ptr<u16[]>(new(cache_align) u16[pixelCount]);

    u32 bytesRead = 0;
    file.Read(pixels.get(), byteCount, bytesRead);

    if (bytesRead != byteCount)
        return false;

    for (int py = 0; py < height; py++)
    {
        const int dstY = y + py;

        if (dstY < 0 || dstY >= 192)
            continue;

        for (int px = 0; px < width; px++)
        {
            const int dstX = x + px;

            if (dstX < 0 || dstX >= 256)
                continue;

            const u16 color = pixels[py * width + px];

            if (useTransparency && color == 0)
                continue;

            DrawBgBufferPixel(dstX, dstY, color);
        }
    }

    return true;
}

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

enum class BatteryDrawState
{
    Percent1,
    Percent25,
    Percent50,
    Percent75,
    Percent100,
    Charging
};

BatteryDrawState ParseBatteryState(u16 batteryState)
{
    const bool charging =
        (batteryState & BATTERY_CHARGER_CONNECTED) != 0;

    if (charging)
        return BatteryDrawState::Charging;

    const u16 level = batteryState & BATTERY_LEVEL_MASK;

    if (level <= 1)
        return BatteryDrawState::Percent1;
    if (level <= 3)
        return BatteryDrawState::Percent25;
    if (level <= 7)
        return BatteryDrawState::Percent50;
    if (level <= 11)
        return BatteryDrawState::Percent75;

    return BatteryDrawState::Percent100;
}

void DsBiosSubBackground::DrawTopBarBatteryIconDsi(u16 batteryState)
{
    constexpr int x = TB_BATT_DS_X_POS;
    constexpr int y = TB_BATT_DS_Y_POS;
    
    const BatteryDrawState state = ParseBatteryState(batteryState);

    const bool chargeBlinkVisible = ((_frameCounter / 30) & 1) == 0;
    const bool lowBlinkVisible = ((_frameCounter / 30) & 1) == 0;

    const u16 PALETTE_BATTERY_DSI_BACK[4] =
    {
        0,
        _palette.black,
        _palette.lightGray,
        _palette.black,
    };

    const u16 PALETTE_BATTERY_DSI_RED[4] =
    {
        0,
        _palette.batteryDsiRed,
        _palette.batteryDsiLightRed,
        _palette.batteryDsiDarkRed,
    };

    const u16 PALETTE_BATTERY_DSI_BLUE[4] =
    {
        0,
        _palette.batteryDsiBlue,
        _palette.batteryDsiLightBlue,
        _palette.batteryDsiDarkBlue,
    };

    const u16 PALETTE_BATTERY_DSI_ORANGE[4] =
    {
        0,
        _palette.batteryDsiOrange,
        _palette.batteryDsiLightOrange,
        _palette.batteryDsiDarkOrange,
    };

    switch (state)
    {
        case BatteryDrawState::Percent1:
            if (chargeBlinkVisible)
            {  
                DrawIndexedIcon(x, y, sBatteryIconDsiBack, BATT_ICON_DSI_W, BATT_ICON_DSI_H, 
                    PALETTE_BATTERY_DSI_BACK);

                DrawIndexedIcon(x, y, sBatteryIconDsi25, BATT_ICON_DSI_W, BATT_ICON_DSI_H,
                    PALETTE_BATTERY_DSI_RED);
            }    
            break;

        case BatteryDrawState::Percent25:
            DrawIndexedIcon(x, y, sBatteryIconDsiBack, BATT_ICON_DSI_W, BATT_ICON_DSI_H, 
                PALETTE_BATTERY_DSI_BACK);

            DrawIndexedIcon(x, y, sBatteryIconDsi25, BATT_ICON_DSI_W, BATT_ICON_DSI_H,
                PALETTE_BATTERY_DSI_RED);
            
            break;

        case BatteryDrawState::Percent50:
            DrawIndexedIcon(x, y, sBatteryIconDsiBack, BATT_ICON_DSI_W, BATT_ICON_DSI_H, 
                PALETTE_BATTERY_DSI_BACK);

            DrawIndexedIcon(x, y, sBatteryIconDsi50, BATT_ICON_DSI_W, BATT_ICON_DSI_H,
                PALETTE_BATTERY_DSI_BLUE);
            
            break;

        case BatteryDrawState::Percent75:
            DrawIndexedIcon(x, y, sBatteryIconDsiBack, BATT_ICON_DSI_W, BATT_ICON_DSI_H, 
                PALETTE_BATTERY_DSI_BACK);

            DrawIndexedIcon(x, y, sBatteryIconDsi75, BATT_ICON_DSI_W, BATT_ICON_DSI_H,
                PALETTE_BATTERY_DSI_BLUE);
            
            break;

        case BatteryDrawState::Percent100:
            DrawIndexedIcon(x, y, sBatteryIconDsiBack, BATT_ICON_DSI_W, BATT_ICON_DSI_H, 
                PALETTE_BATTERY_DSI_BACK);

            DrawIndexedIcon(x, y, sBatteryIconDsi100, BATT_ICON_DSI_W, BATT_ICON_DSI_H,
                PALETTE_BATTERY_DSI_BLUE);
            
            break;

        case BatteryDrawState::Charging:            
            DrawIndexedIcon(x, y, sBatteryIconDsiBack, BATT_ICON_DSI_W, BATT_ICON_DSI_H, 
                PALETTE_BATTERY_DSI_BACK);

            DrawIndexedIcon(x, y, sBatteryIconDsi100, BATT_ICON_DSI_W, BATT_ICON_DSI_H,
                PALETTE_BATTERY_DSI_ORANGE);
            
                if (chargeBlinkVisible)
                {  
                    DrawIndexedIcon(x, y, sBatteryIconDsiPlug, BATT_ICON_DSI_W, BATT_ICON_DSI_H, 
                                PALETTE_BATTERY_DSI_BACK);
                }
            break;
    }
}


void DsBiosSubBackground::DrawTopBarBatteryIcon(bool lowBattery)
{
    constexpr int x = TB_BATT_DS_X_POS;
    constexpr int y = TB_BATT_DS_Y_POS;

    const u16 PALETTE_BATTERY_GOOD[4] =
    {
        0,
        _palette.black,
        _palette.batteryDarkGreen,
        _palette.batteryGreen,
    };

    const u16 PALETTE_BATTERY_LOW[4] =
    {
        0,
        _palette.black,
        _palette.batteryDarkRed,
        _palette.batteryRed,
    };

    const bool lowBlinkVisible = ((_frameCounter / 30) & 1) == 0;


    if (lowBattery)
    {
        if (!lowBlinkVisible)
            return; // skip drawing = invisible frame
    }

    DrawIndexedIcon(
        x, y,
        sBatteryIcon,
        BATT_ICON_W, BATT_ICON_H,
        lowBattery ? PALETTE_BATTERY_LOW : PALETTE_BATTERY_GOOD);

}

// ########## ANALOG CLOCK ##########

void DsBiosSubBackground::DrawHand(int x0, int y0, int x1, int y1, u16 color, int thickness)
{
    int half = thickness / 2;

    auto DrawBrush = [&](int cx, int cy)
    {
        int start = -(thickness / 2);
        int end   = start + thickness - 1;

        for (int oy = start; oy <= end; oy++)
        {
            for (int ox = start; ox <= end; ox++)
            {
                DrawBmpPixel(cx + ox, cy + oy, color);
            }
        }
    };

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        DrawBrush(x0, y0);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void DsBiosSubBackground::DrawClockHands(int h, int m, int s, int alarmHour, int alarmMinute)
{
    // alarm
    float alarmAngle = ((alarmHour % 12) + alarmMinute / 60.0f)
                        * (M_TWOPI / 12.0f)
                        - M_PI_2;

    DrawHand(CLOCK_CX, CLOCK_CY,
        CLOCK_CX + (int)(cosf(alarmAngle) * ALARM_LEN),
        CLOCK_CY + (int)(sinf(alarmAngle) * ALARM_LEN),
        _palette.alarmHand,
        2);

    // hours
    float hAngle = ((h % 12) + m / 60.0f) * (M_TWOPI / 12.0f) - M_PI_2;
    DrawHand(CLOCK_CX, CLOCK_CY,
        CLOCK_CX + (int)(cosf(hAngle) * HOUR_LEN),
        CLOCK_CY + (int)(sinf(hAngle) * HOUR_LEN),
        _palette.mediumGray, 2);

    // minutes
    float mAngle = (m + s / 60.0f) * (M_TWOPI / 60.0f) - M_PI_2;
    DrawHand(CLOCK_CX, CLOCK_CY,
        CLOCK_CX + (int)(cosf(mAngle) * MIN_LEN),
        CLOCK_CY + (int)(sinf(mAngle) * MIN_LEN),
        _palette.mediumGray, 2);

    // seconds
    float sAngle = s * (M_TWOPI / 60.0f) - M_PI_2;
    DrawHand(CLOCK_CX, CLOCK_CY,
        CLOCK_CX + (int)(cosf(sAngle) * SEC_LEN),
        CLOCK_CY + (int)(sinf(sAngle) * SEC_LEN),
        _palette.user.base, 2);

    // cap
    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            DrawBmpPixel(CLOCK_CX + x, CLOCK_CY + y, _palette.darkGray);
        }
    }
}

// ########## CALENDAR ##########

void DsBiosSubBackground::DrawBox(
    int x, int y, int w, int h,
    u16 borderA, u16 borderB, u16 fillColor)
{
    for (int py = 0; py < h; py++)
    {
        for (int px = 0; px < w; px++)
        {
            const bool isBorder =
                px == 0 || py == 0 || px == w - 1 || py == h - 1;

            if (!isBorder)
            {
                DrawBmpPixel(x + px, y + py, fillColor);
                continue;
            }

            // ---- Corner check ----
            const bool isCorner =
                (px == 0 && py == 0) ||
                (px == w - 1 && py == 0) ||
                (px == 0 && py == h - 1) ||
                (px == w - 1 && py == h - 1);

            if (isCorner)
            {
                DrawBmpPixel(x + px, y + py, borderA);
                continue;
            }

            // ---- Edge position (offset so corners don't break pattern) ----
            int t = 0;

            if (py == 0)               t = px - 1;           // top
            else if (px == w - 1)      t = py - 1;           // right
            else if (py == h - 1)      t = px - 1;           // bottom
            else if (px == 0)          t = py - 1;           // left

            u16 color = (t % 2 == 0) ? borderA : borderB;

            DrawBmpPixel(x + px, y + py, color);
        }
    }
}

void DsBiosSubBackground::DrawCalendarHeader(int month, int year){
    constexpr int x = 157;
    constexpr int y = 33;

    u16 color = _palette.black;

    char text[9];

    snprintf(
        text,
        sizeof(text),
        "%02d/%04d",
        month,
        year);

    DrawTextBig(x, y, text, color, this);
}

void DsBiosSubBackground::DrawCalendarNumber(
    int cellX, int cellY, int number, u16 color)
{
    char text[3];

    if (number >= 10)
    {
        text[0] = '0' + (number / 10);
        text[1] = '0' + (number % 10);
        text[2] = '\0';
    }
    else
    {
        text[0] = '0' + number;
        text[1] = '\0';
    }

    constexpr int glyphH = 8;

    const int textW = GetTextWidth(text);
    const int drawX = cellX + (CAL_CELL_W - textW) / 2;
    const int drawY = cellY + (CAL_CELL_H - glyphH) / 2;

    DrawText(drawX, drawY, text, color, this);
}

int DsBiosSubBackground::DaysInMonth(int y, int m)
{
    static const int days[] =
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)))
        return 29;
    return days[m - 1];
}

void DsBiosSubBackground::DrawCalendar(int year, int month, int today)
{
    constexpr int CAL_ROWS = 5;

    struct tm firstDay = _tm;
    firstDay.tm_mday = 1;
    mktime(&firstDay);

    const int startDow = firstDay.tm_wday; // 0 = Sun
    const int numDays  = DaysInMonth(year, month);

    for (int d = 1; d <= numDays; d++)
    {
        const int index = startDow + d - 1;

        const int col = index % 7;
        int row = index / 7;

        // Wrap 6th calendar row back to the first row.
        if (row >= CAL_ROWS)
            row = 0;

        const int px = CAL_ORIGIN_X + col * CAL_CELL_W;
        const int py = CAL_ORIGIN_Y + row * CAL_CELL_H;

        if (d == today)
        {
            constexpr int boxSize = 13;

            const int boxX = px + (CAL_CELL_W - boxSize) / 2;
            const int boxY = py + (CAL_CELL_H - boxSize) / 2;

            DrawBox(boxX, boxY, boxSize, boxSize,
                    _palette.user.dark,
                    _palette.user.light2,
                    _palette.user.light6);
        }

        const u16 color =
            (d == today) ? _palette.black    :
            (col == 0)   ? _palette.sunday   :
            (col == 6)   ? _palette.saturday :
                           _palette.black;

        DrawCalendarNumber(px, py, d, color);

    }
    DrawCalendarHeader(month, year);
}
