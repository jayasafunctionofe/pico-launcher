#pragma once

#include "../custom/CustomTheme.h"

class DsBiosTheme : public CustomTheme
{
public:
    DsBiosTheme(const TCHAR* folderName,
                const Rgb<8,8,8>& primaryColor,
                bool darkMode);

    void LoadRomBrowserResources(
        const VramContext& mainVramContext,
        const VramContext& subVramContext) override;

    std::unique_ptr<IThemeBackground> CreateRomBrowserTopBackground() const override;

private:

};