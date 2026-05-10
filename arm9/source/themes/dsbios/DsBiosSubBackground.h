#pragma once
#include "../background/IThemeBackground.h"
#include "../IFontRepository.h"

class DsBiosSubBackground : public IThemeBackground
{
public:

    explicit DsBiosSubBackground(const IFontRepository* fontRepository)
        : _fontRepository(fontRepository) {}

    void LoadResources(
        const ITheme& theme, const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

private:
;
    const IFontRepository* _fontRepository;

};
