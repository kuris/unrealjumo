using UnrealBuildTool;

public class SimpleWeather : ModuleRules
{
    public SimpleWeather(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "Niagara", "Json", "Projects", "Water" });
        RuntimeDependencies.Add("$(PluginDir)/Data/Dialogues.json", StagedFileType.UFS);
        RuntimeDependencies.Add("$(PluginDir)/Data/Fonts/ChosunGs.TTF", StagedFileType.UFS);
        RuntimeDependencies.Add("$(PluginDir)/Data/Fonts/LICENSE-Chosun.txt", StagedFileType.UFS);
    }
}
