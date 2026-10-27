using UnrealBuildTool;

public class SimpleWeatherEditor : ModuleRules
{
    public SimpleWeatherEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "SimpleWeather" });
    }
}
