#pragma once
#include "../background/IThemeBackground.h"
#include "../IFontRepository.h"

#include <memory>
#include <ctime>
#include "DsBiosSystemInfo.h"
#include "DsColorUtils.h"
#include <nds/system.h>

#include "DsFont.h"
#include "DsIcons.h"

class DsBiosSubBackground : public IThemeBackground
{
public:

    explicit DsBiosSubBackground(bool hasCustomSystemColor,
                                Rgb<8,8,8> customSystemColor,
                                const IFontRepository* fontRepository)
        : _hasCustomSystemColor (hasCustomSystemColor)
        , _customSystemColor (customSystemColor)
        , _fontRepository(fontRepository) {}

    void LoadResources(
        const ITheme& theme, const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    // Bitmap helper - public
    void DrawBmpPixel(int px, int py, u16 color);

private:
    const IFontRepository* _fontRepository;

    DsBiosSystemInfo::Settings _systemSettings;
    bool _hasCustomSystemColor;
    Rgb8 _customSystemColor;

    // get a palette we can use for objects that
    // change based on user's favorite color
    UiPalette _palette = MakeUiPalette(THEME_USER_PALETTES[0]);    

    // RAM copy of the unmodified background for region restoration
    std::unique_ptr<u16[]> _bgBuffer; // 256 * 192 u16s = 98 304 bytes
    
    // clock
    struct tm _tm {};
    int _prevSecond = -1;
    int _frameCounter = 0;
    bool _prevColonVisible = false;

    // topbar
    static constexpr int TB_TEXT_Y_POS = 3;
    static constexpr int TB_USER_X_POS = 8;
    static constexpr int TB_CLOCK_X_POS = 148;
    static constexpr int TB_DATE_X_POS = 178;

    // analogue clock
    static constexpr int CLOCK_CX     = 63;
    static constexpr int CLOCK_CY     = 95;
    static constexpr int CLOCK_W      = 96;
    
    static constexpr int HOUR_LEN     = 24;
    static constexpr int MIN_LEN      = 32;
    static constexpr int SEC_LEN      = 36;
    static constexpr int ALARM_LEN    = 15;
    static constexpr int CLOCK_RESTORE_R = SEC_LEN + 2;

    // calendar
    static constexpr int CAL_ORIGIN_X = 128;
    static constexpr int CAL_ORIGIN_Y = 64;
    static constexpr int CAL_CELL_W   = 16;
    static constexpr int CAL_CELL_H   = 16;

    // Bitmap helpers 
    vu16* BmpVram() const;
    void RestoreBgRegion(int x, int y, int w, int h);
    void DrawIndexedIcon(int x, int y, const u8* icon, int w, int h, const u16 palette[4]);

    // Top Bar stuff
    void DrawTopBar();
    void DrawTopBarBackground();
    void DrawTopBarDividers();
    void DrawTopBarUserName(const std::string& userName);
    void DrawDigitalClock(int hour, int minute);
    void DrawTopBarDate(int month, int day);


};
