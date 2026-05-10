#include "common.h"
#include "material/MaterialTheme.h"
#include "custom/CustomTheme.h"
#include  "dsbios/DsBiosTheme.h"

#include "ThemeFactory.h"

std::unique_ptr<ITheme> ThemeFactory::CreateFromThemeInfo(const ThemeInfo* themeInfo) const
{
    switch (themeInfo->GetType())
    {
        case ThemeType::Material:
        {
            return std::make_unique<MaterialTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                themeInfo->GetIsDarkTheme());
        }
        case ThemeType::Custom:
        {
            return std::make_unique<CustomTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                themeInfo->GetIsDarkTheme());
        }
        case ThemeType::DsBios:
        {
            return std::make_unique<DsBiosTheme>(
                themeInfo->GetFolderName(),
                themeInfo->GetPrimaryColor(),
                themeInfo->GetIsDarkTheme());
        }
        default:
        {
            return nullptr;
        }
    }
}