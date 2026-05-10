#pragma once
#include "../background/IThemeBackground.h"
#include "../IFontRepository.h"

#include "DsBiosSystemInfo.h"
#include "DsColorUtils.h"
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

private:
;
    const IFontRepository* _fontRepository;

    DsBiosSystemInfo::Settings _systemSettings;
    bool _hasCustomSystemColor;
    Rgb8 _customSystemColor;

    // get a palette we can use for objects that
    // change based on user's favorite color
    UiPalette _palette = MakeUiPalette(THEME_USER_PALETTES[0]);
    
};
