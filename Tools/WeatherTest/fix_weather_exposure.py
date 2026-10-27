"""Run inside Unreal's Python console with LV_WeatherTest open and PIE stopped."""
import unreal


def main():
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor.get_editor_world()
    if not world or world.get_path_name() != "/Game/Maps/LV_WeatherTest.LV_WeatherTest":
        raise RuntimeError("Open /Game/Maps/LV_WeatherTest first. No files were changed.")
    if editor.get_game_world():
        raise RuntimeError("Stop Play before applying the exposure fix.")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    label = "Weather Test Exposure"
    matches = [a for a in actors.get_all_level_actors()
               if isinstance(a, unreal.PostProcessVolume) and a.get_actor_label() == label]
    with unreal.ScopedEditorTransaction("Fix weather test map exposure"):
        volume = matches[0] if matches else actors.spawn_actor_from_class(
            unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
        if not volume:
            raise RuntimeError("Could not create the test exposure volume.")
        volume.modify()
        volume.set_actor_label(label)
        volume.set_editor_property("unbound", True)
        volume.set_editor_property("enabled", True)
        volume.set_editor_property("priority", 100.0)
        volume.set_editor_property("blend_weight", 1.0)
        settings = volume.get_editor_property("settings")
        for name, value in {
            "override_auto_exposure_method": True,
            "auto_exposure_method": unreal.AutoExposureMethod.AEM_HISTOGRAM,
            "override_auto_exposure_min_brightness": True,
            "override_auto_exposure_max_brightness": True,
            "auto_exposure_min_brightness": 12.0,
            "auto_exposure_max_brightness": 12.0,
            "override_auto_exposure_bias": True,
            "auto_exposure_bias": 0.0,
        }.items():
            settings.set_editor_property(name, value)
        volume.set_editor_property("settings", settings)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.save_current_level():
        raise RuntimeError("Exposure applied but map save failed. Save LV_WeatherTest manually.")
    unreal.log("WEATHER_EXPOSURE_FIX_APPLIED: saved LV_WeatherTest at fixed EV100 12. Play and press R to test.")


main()
