#include "IslandUIStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Fonts/CompositeFont.h"
FSlateFontInfo IslandUIStyle::Font(int32 Size)
{
    static const TSharedPtr<const FCompositeFont> Composite = []() -> TSharedPtr<const FCompositeFont>
    {
        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SimpleWeather"));
        if (!Plugin) return nullptr;
        return MakeShared<FCompositeFont>(FName(TEXT("Regular")), Plugin->GetBaseDir() / TEXT("Data/Fonts/ChosunGs.TTF"),
            EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
    }();
    return Composite ? FSlateFontInfo(Composite, Size, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
}
