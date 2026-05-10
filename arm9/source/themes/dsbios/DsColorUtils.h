#pragma once

#include <cstdint>
#include <nds/ndstypes.h>

static constexpr u16 COL_OPAQUE = 0x8000;

static constexpr u16 HexToBGR555(u32 hex)
{
    const u16 r = ((hex >> 16) & 0xFF) >> 3;
    const u16 g = ((hex >> 8)  & 0xFF) >> 3;
    const u16 b = ((hex >> 0)  & 0xFF) >> 3;

    return (b << 10) | (g << 5) | r;
}

static constexpr u16 Opaque(u32 hex)
{
    return COL_OPAQUE | HexToBGR555(hex);
}

// ########## Palette of colors based on user color ##########

struct UserPalette
{
    u16 dark;
    u16 base;
    u16 light1;
    u16 light2;
    u16 light3;
    u16 light4;
    u16 light5;
    u16 light6;
};

// ########## Palette for other UI stuff ##########

struct UiPalette
{
    u16 black;
    u16 darkGray;
    u16 mediumGray;
    u16 lightGray;
    u16 white;

    u16 batteryDarkGreen;
    u16 batteryGreen;
    u16 batteryDarkRed;
    u16 batteryRed;
    u16 batteryDsiDarkOrange;
    u16 batteryDsiOrange;
    u16 batteryDsiLightOrange;
    u16 batteryDsiDarkBlue;
    u16 batteryDsiBlue;
    u16 batteryDsiLightBlue;
    u16 batteryDsiDarkRed;
    u16 batteryDsiRed;
    u16 batteryDsiLightRed;

    u16 alarmHand;
    u16 gbaOrange;
    u16 sunday;
    u16 saturday;

    UserPalette user;
};

// list of colors that stem from users color
// index 1 is the base color the user selects
// the others are picked from the gradient
// at the top of the screen, and calendar display
inline constexpr UserPalette THEME_USER_PALETTES[16] =
{
    // 0 = Gray
    { Opaque(0X416179), Opaque(0X61829A), Opaque(0X698AA2), Opaque(0X7192A2), Opaque(0X8AA2B2), Opaque(0XA2BAC3), Opaque(0XBACBD3), Opaque(0XD3DBE3) },
	// 1 = Brown
    { Opaque(0X8A3000), Opaque(0XBA4900), Opaque(0XCB5100), Opaque(0XDB5900), Opaque(0XDB7120), Opaque(0XE38A49), Opaque(0XEBA269), Opaque(0XF3B28A) },
    // 2 = Red
    { Opaque(0XCB0000), Opaque(0XFB0018), Opaque(0XFB3849), Opaque(0XFB5159), Opaque(0XFB6971), Opaque(0XFB7982), Opaque(0XFB929A), Opaque(0XFBA2AA) },
	// 3 = Pink
    { Opaque(0XDB4982), Opaque(0XFB6971), Opaque(0XFBA2FB), Opaque(0XFBB2FB), Opaque(0XFBC3FB), Opaque(0XFBCBFB), Opaque(0XFBD3FB), Opaque(0XFBDBFB) },
	// 4 = Orange
    { Opaque(0XDB5100), Opaque(0XFB9200), Opaque(0XFBA218), Opaque(0XFBAA38), Opaque(0XFBBA59), Opaque(0XFBC371), Opaque(0XFBD392), Opaque(0XFBDBAA) },
	// 5 = Yellow
    { Opaque(0XC38A00), Opaque(0XF3E300), Opaque(0XFBF330), Opaque(0XFBFB61), Opaque(0XFBFB71), Opaque(0XFBFB8A), Opaque(0XFBFB9A), Opaque(0XFBFBAA) },
	// 6 = Yellow/Green-ish
    { Opaque(0X69BA00), Opaque(0XAAFB00), Opaque(0XD5FD80), Opaque(0XAAFB61), Opaque(0XBAFB79), Opaque(0XCBFB9A), Opaque(0XD3FBB2), Opaque(0XE3FBCB) },
	// 7 = Green
    { Opaque(0X00B200), Opaque(0X00FB00), Opaque(0X30FB30), Opaque(0X59FB59), Opaque(0X71FB71), Opaque(0X92FB92), Opaque(0XAAFBAA), Opaque(0XCBFBCB) },
	// 8 = Dark Green
    { Opaque(0X009A00), Opaque(0X00A238), Opaque(0X00B238), Opaque(0X00C341), Opaque(0X28D361), Opaque(0X51DB82), Opaque(0X82E3A2), Opaque(0XAAEBC3) },
	// 9 = Green/Blue-ish
    { Opaque(0X188241), Opaque(0X49DB8A), Opaque(0X49E392), Opaque(0X49EB92), Opaque(0X69EBA2), Opaque(0X82F3B2), Opaque(0X9AF3C3), Opaque(0XBAFBD3) },
	// 10 = Light Blue
    { Opaque(0X1871A2), Opaque(0X30BAF3), Opaque(0X38C3FB), Opaque(0X41CBFB), Opaque(0X61D3FB), Opaque(0X82DBFB), Opaque(0XA2E3FB), Opaque(0XC3EBFB) },
	// 11 = Blue
    { Opaque(0X0038C3), Opaque(0X0059F3), Opaque(0X1869FB), Opaque(0X1869FB), Opaque(0X519AFB), Opaque(0X69AAFB), Opaque(0X8ABAFB), Opaque(0XA2CBFB) },
	// 12 = Dark Blue
    { Opaque(0X0000AA), Opaque(0X000092), Opaque(0X0008AA), Opaque(0X0010C3), Opaque(0X2030D3), Opaque(0X4959DB), Opaque(0X7179E3), Opaque(0X9AA2EB) },
	// 13 = Dark Purple
    { Opaque(0X710092), Opaque(0X8A00D3), Opaque(0XA210EB), Opaque(0XB228FB), Opaque(0XC341FB), Opaque(0XCB61FB), Opaque(0XD382FB), Opaque(0XDBA2FB) },
	// 14 = Purple
    { Opaque(0XBA309A), Opaque(0XD300EB), Opaque(0XDB20F3), Opaque(0XE341F3), Opaque(0XE361F3), Opaque(0XF3A2FB), Opaque(0XF3A2FB), Opaque(0XF3C3FB) },
	// 15 = Purple/Red-ish
    { Opaque(0XDB0082), Opaque(0XFB0092), Opaque(0XFB30AA), Opaque(0XFB49B2), Opaque(0XFB61C3), Opaque(0XFB79CB), Opaque(0XFB92D3), Opaque(0XFBAADB) },
};

inline constexpr UiPalette MakeUiPalette(UserPalette user)
{
    return UiPalette {
        .black            = Opaque(0x000000),
        .darkGray         = Opaque(0x494949),
        .mediumGray       = Opaque(0x797979),
        .lightGray        = Opaque(0xC3C3C3),
        .white            = Opaque(0xFFFFFF),

        .batteryDarkGreen = Opaque(0x288238),
        .batteryGreen     = Opaque(0x30F349),
        .batteryDarkRed   = Opaque(0xF30020),
        .batteryRed       = Opaque(0xFB5116),

        .batteryDsiDarkOrange   = Opaque(0xFB2000),
        .batteryDsiOrange       = Opaque(0xFB6928),
        .batteryDsiLightOrange  = Opaque(0xFB9A49),
        
        .batteryDsiDarkBlue     = Opaque(0x0051B2),
        .batteryDsiBlue         = Opaque(0x0071FB),
        .batteryDsiLightBlue    = Opaque(0x20AAFB),

        .batteryDsiDarkRed      = Opaque(0xCB0000),
        .batteryDsiRed          = Opaque(0xF30000),
        .batteryDsiLightRed     = Opaque(0xFB5151),

        .alarmHand        = Opaque(0xFB5971),
        .gbaOrange        = Opaque(0xFB6900),
        .sunday           = Opaque(0x790000),
        .saturday         = Opaque(0x000082),

        .user             = user,
    };
}

static constexpr u8 Clamp255(int value)
{
    return value < 0 ? 0 : value > 255 ? 255 : value;
}

static constexpr u32 RgbToHex(u8 r, u8 g, u8 b)
{
    return (static_cast<u32>(r) << 16)
         | (static_cast<u32>(g) << 8)
         |  static_cast<u32>(b);
}

static inline uint16_t HexToBGR555_Rounded(uint32_t hex)
{
    uint8_t r = (hex >> 16) & 0xFF;
    uint8_t g = (hex >> 8)  & 0xFF;
    uint8_t b = (hex >> 0)  & 0xFF;

    // Round instead of truncate
    uint16_t r5 = (r * 31 + 127) / 255;
    uint16_t g5 = (g * 31 + 127) / 255;
    uint16_t b5 = (b * 31 + 127) / 255;

    return (b5 << 10) | (g5 << 5) | r5;
}

static constexpr u16 OpaqueRounded(u32 hex)
{
    return COL_OPAQUE | HexToBGR555_Rounded(hex);
}

// if the user selects a custom color
// this is a fairly close approx of the
// hand pick palette colors in the stock DS BIOS
static constexpr UserPalette BaseToUserPalette(Rgb8 base)
{
    const int r = base.r;
    const int g = base.g;
    const int b = base.b;

    auto make = [](int r, int g, int b)
    {
        return OpaqueRounded(RgbToHex(
            Clamp255(r),
            Clamp255(g),
            Clamp255(b)
        ));
    };

    return UserPalette {
        // Dark
        make(r - 29, g - 34, b - 27),

        // Base
        make(r, g, b),

        // Lights
        make(r + 12, g + 20, b + 34),
        make(r + 15, g + 32, b + 44),
        make(r + 32, g + 50, b + 60),
        make(r + 49, g + 67, b + 77),
        make(r + 65, g + 81, b + 91),
        make(r + 81, g + 95, b + 105),
    };
}