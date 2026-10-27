"""Attach Fab Weather System to the weather island and verify its real outputs."""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import shutil
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
output = root / 'Saved/WeatherSystem'
output.mkdir(parents=True, exist_ok=True)
baseline = json.loads((root / 'Tools/WeatherTest/protected_baseline.json').read_text())


def verify_protected():
    for name, expected in baseline.items():
        assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name


verify_protected()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/LV_BeachIsland_Weather')
existing = list(actors.get_all_level_actors())
controller = next(a for a in existing if a.get_class().get_path_name() == '/Script/SimpleWeather.SimpleWeatherActor')
patrols = [a for a in existing if a.get_class().get_path_name() == '/Script/SimpleWeather.IslandPatrolCharacter']
assert len(patrols) == 2
source_map = root / 'Content/Maps/LV_BeachIsland_Weather.umap'
backup = output / 'MapBackups' / datetime.now().strftime('%Y%m%d-%H%M%S')
backup.mkdir(parents=True)
shutil.copy2(source_map, backup / source_map.name)
cls = unreal.load_class(None, '/Game/WeatherSystem/BluePrints/WeatherManagmentBP.WeatherManagmentBP_C')
assert cls
weather = next((a for a in existing if a.get_class() == cls), None)
if weather is None:
    start = next(a for a in existing if isinstance(a, unreal.PlayerStart))
    weather = actors.spawn_actor_from_class(cls, start.get_actor_location())
weather.set_actor_label('Weather System - Niagara Rain')
weather.set_editor_property('auto_receive_input', unreal.AutoReceiveInput.DISABLED)
weather.set_editor_property('RainOn?', False)
weather.call_method('SetRainComponentState', (False,))
weather.call_method('SetRain', (0.0,))
weather.call_method('SetHumidity', (0.0,))
controller.set_editor_property('weather_system_actor', weather)
controller.set_actor_label('Beach Weather Controls - Fab Weather System')
controller.set_editor_property('start_clear', True)
# Remove serialized legacy drops; OnConstruction also skips them after reload.
rain = controller.get_editor_property('rain')
rain.clear_instances()
rain.set_visibility(False)
niagara = weather.get_component_by_class(unreal.NiagaraComponent)
collection = unreal.load_asset('/Game/WeatherSystem/Materials/MaterialParameterCollections/GeneralMPC')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
checks = []
for state, amount in [(unreal.SimpleWeather.RAIN, 0.0), (unreal.SimpleWeather.RAIN, 0.5),
                      (unreal.SimpleWeather.RAIN, 1.0), (unreal.SimpleWeather.CLEAR, 1.0)]:
    controller.set_rain_amount(amount)
    controller.set_weather(state)
    expected = amount if state == unreal.SimpleWeather.RAIN else 0.0
    actual, valid = niagara.get_variable_float('User.SpawnRateMultiplyer')
    assert valid and abs(actual - expected) < 0.001, (actual, expected)
    assert niagara.is_visible() == (expected > 0.0)
    humidity = unreal.MaterialLibrary.get_scalar_parameter_value(world, collection, 'Humidity')
    assert abs(humidity - expected) < 0.001, (humidity, expected)
    assert not rain.is_visible() and rain.get_instance_count() == 0
    checks.append({'weather': str(state), 'amount': amount, 'spawn_multiplier': actual, 'humidity': humidity})
controller.set_rain_amount(1.0)
controller.set_weather(unreal.SimpleWeather.CLEAR)
assert levels.save_current_level()
verify_protected()
report = {'map': '/Game/Maps/LV_BeachIsland_Weather', 'blueprint': cls.get_path_name(),
          'checks': checks, 'map_backup': str(backup), 'patrols_preserved': len(patrols),
          'protected_files_unchanged': True, 'mac_runtime_validation': 'pending'}
(output / 'application_report.json').write_text(json.dumps(report, indent=2))
actors.set_selected_level_actors([weather, controller])
unreal.log('WEATHER_SYSTEM_APPLICATION_PASS')
