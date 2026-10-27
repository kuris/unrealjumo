using UnrealBuildTool;

// A project game target is required to link native project plugins into the
// monolithic Android executable instead of reusing the stock UnrealGame binary.
public class buildtestTarget : TargetRules
{
    public buildtestTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("buildtest");
        // SimpleWeather is enabled by its descriptor. EnablePlugins here would
        // require rebuilding shared engine modules in an installed engine.
    }
}
