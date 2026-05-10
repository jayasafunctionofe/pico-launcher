#include "common.h"
#include "json/ArduinoJson.h"
#include "DsBiosTheme.h"
#include "DsBiosSubBackground.h"

#define JSON_RESERVED_SIZE  4096
#define KEY_COLOR_R     "r"
#define KEY_COLOR_G     "g"
#define KEY_COLOR_B     "b"

#define KEY_CUSTOM_SYSTEM_COLOR     "customSystemColor"

static Rgb8 parseColor(const JsonObjectConst& json, const Rgb8& defaultColor)
{
    if (json.isNull())
    {
        return defaultColor;
    }

    return Rgb8(
        json[KEY_COLOR_R] | 0,
        json[KEY_COLOR_G] | 0,
        json[KEY_COLOR_B] | 0
    );
}

DsBiosTheme::DsBiosTheme(const TCHAR* folderName,
                         const Rgb<8,8,8>& primaryColor,
                         bool darkMode)
    : CustomTheme(folderName, primaryColor, darkMode)
{
}

void DsBiosTheme::LoadRomBrowserResources(
    const VramContext& mainVramContext,
    const VramContext& subVramContext)
{
    CustomTheme::LoadRomBrowserResources(mainVramContext, subVramContext);

    const auto file = std::make_unique<File>();

    if (!OpenThemeFile(*file, "theme.json"))
    {
        return;
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
        return;

    const JsonObjectConst customColorJson = json[KEY_CUSTOM_SYSTEM_COLOR];
    _hasCustomSystemColor = !customColorJson.isNull();
    _customSystemColor = parseColor(
        customColorJson,
        Rgb8(96, 128, 152)
    );        
}

std::unique_ptr<IThemeBackground> DsBiosTheme::CreateRomBrowserTopBackground() const
{
    return std::make_unique<DsBiosSubBackground>(        
        _hasCustomSystemColor,
        _customSystemColor,
        GetFontRepository()
    );
}