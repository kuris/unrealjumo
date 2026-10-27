"""Run in Unreal's Cmd console: py \"Tools/WeatherTest/create_beach_weather.py\".

Creates LV_BeachIsland_Weather from the saved beach map using Unreal's template
loader. Never saves the source level or changes project settings.
"""
import hashlib
import json
from pathlib import Path

import unreal


SOURCE = "/Game/Maps/LV_BeachIsland"
DESTINATION = "/Game/Maps/LV_BeachIsland_Weather"


def exposure_settings(settings):
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
    return settings


def main():
    root = Path(unreal.Paths.project_dir()).resolve()
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if editor.get_game_world():
        raise RuntimeError("Stop Play first. No map was changed.")
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError("Save your current level changes first, then run again.")
    if unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError("Save your current asset changes first, then run again.")
    if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION):
        raise RuntimeError("LV_BeachIsland_Weather already exists; refusing to overwrite it.")
    weather_class = unreal.load_class(None, "/Script/SimpleWeather.SimpleWeatherActor")
    if not weather_class:
        raise RuntimeError("SimpleWeather plugin must be compiled and enabled first.")
    source_file = root / "Content/Maps/LV_BeachIsland.umap"
    if not source_file.is_file():
        raise RuntimeError("Saved LV_BeachIsland.umap was not found.")
    baseline = json.loads((root / "Tools/WeatherTest/protected_baseline.json").read_text())
    baseline.setdefault("Content/Maps/LV_BeachIsland.umap", hashlib.sha256(source_file.read_bytes()).hexdigest())
    for name, expected in baseline.items():
        if hashlib.sha256((root / name).read_bytes()).hexdigest() != expected:
            raise RuntimeError("Protected file differs from baseline: " + name)

    # UE loads the source as an untitled template and saves a separate package.
    if not levels.new_level_from_template(DESTINATION, SOURCE):
        raise RuntimeError("Unreal could not duplicate the beach map. See Output Log.")
    world = editor.get_editor_world()
    if world.get_path_name() != DESTINATION + ".LV_BeachIsland_Weather":
        raise RuntimeError("Destination world check failed; refusing to edit this world.")
    originals = list(actors.get_all_level_actors())
    if any(a.get_level() != levels.get_current_level() for a in originals):
        raise RuntimeError("Copy contains shared sublevels. Stopped before changing actors to protect source sublevels.")
    start = next((a for a in originals if isinstance(a, unreal.PlayerStart)), None)
    cameras = [a for a in originals if isinstance(a, unreal.CameraActor)]
    active_camera = next((a for a in cameras if a.get_editor_property(
        "auto_activate_for_player") == unreal.AutoReceiveInput.PLAYER0), None)
    anchor = active_camera or start
    if not anchor:
        raise RuntimeError("No PlayerStart or auto-active camera found in the copy. Place a PlayerStart before applying weather.")
    location = anchor.get_actor_location()
    weather_location = unreal.Vector(location.x, location.y, location.z - 200.0)
    lighting_types = (unreal.DirectionalLightComponent, unreal.SkyLightComponent,
                      unreal.SkyAtmosphereComponent, unreal.ExponentialHeightFogComponent)
    old_sun_rotation = None
    disabled = []
    with unreal.ScopedEditorTransaction("Apply weather to beach copy"):
        # Hide only environmental components in the copy, preserving geometry,
        # water, materials, gameplay actors, and the source level.
        for actor in originals:
            for component_type in lighting_types:
                for component in actor.get_components_by_class(component_type):
                    if isinstance(component, unreal.DirectionalLightComponent) and old_sun_rotation is None:
                        old_sun_rotation = component.get_world_rotation()
                    component.modify()
                    component.set_visibility(False)
                    component.set_hidden_in_game(True)
                    disabled.append(component.get_path_name())
        weather = actors.spawn_actor_from_class(weather_class, weather_location)
        if not weather:
            raise RuntimeError("Could not place weather actor in the beach copy.")
        weather.set_actor_label("Beach Weather - R toggles Clear / Rain")
        weather.set_editor_property("enable_test_key", True)
        if old_sun_rotation is not None:
            weather.get_editor_property("sun").set_world_rotation(old_sun_rotation, False, False)
        weather.set_weather(unreal.SimpleWeather.CLEAR)
        volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector())
        volume.set_actor_label("Beach Weather Exposure")
        volume.set_editor_property("unbound", True)
        volume.set_editor_property("priority", 1000.0)
        volume.set_editor_property("blend_weight", 1.0)
        volume.set_editor_property("settings", exposure_settings(volume.get_editor_property("settings")))
        for actor in originals:
            for camera in actor.get_components_by_class(unreal.CameraComponent):
                camera.modify()
                camera.set_editor_property("post_process_settings", exposure_settings(
                    camera.get_editor_property("post_process_settings")))
        rain = weather.get_editor_property("rain")
        weather.toggle_weather()
        if weather.get_weather() != unreal.SimpleWeather.RAIN or not rain.is_visible() or rain.get_instance_count() != 400:
            raise RuntimeError("Rain state validation failed; the copy has not been saved with weather.")
        weather.toggle_weather()
        if weather.get_weather() != unreal.SimpleWeather.CLEAR or rain.is_visible():
            raise RuntimeError("Clear state validation failed.")
    if not levels.save_current_level():
        raise RuntimeError("Could not save the weather copy.")
    for name, expected in baseline.items():
        if hashlib.sha256((root / name).read_bytes()).hexdigest() != expected:
            raise RuntimeError("Protected file changed: " + name)
    report = {
        "source": SOURCE, "destination": DESTINATION,
        "original_actor_count": len(originals),
        "disabled_environment_components_in_copy": disabled,
        "rain_anchor": anchor.get_actor_label(),
        "rain_area_cm": [2000, 2000],
        "rain_instances": rain.get_instance_count(),
        "protected_files_unchanged": list(baseline),
        "state_checks": "clear -> rain -> clear passed",
        "visual_play_test": "pending",
    }
    (root / "Tools/WeatherTest/beach_weather_report.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False))
    actors.set_selected_level_actors([weather])
    unreal.log("BEACH_WEATHER_PASS: copied LV_BeachIsland_Weather; original beach unchanged. Play, click viewport, press R.")


main()
