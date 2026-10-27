using UnrealBuildTool;

// AutomationTool requires an editor target when it discovers a project game
// target, even when packaging skips rebuilding the installed editor.
public class buildtestEditorTarget : TargetRules
{
    public buildtestEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("buildtest");
    }
}
