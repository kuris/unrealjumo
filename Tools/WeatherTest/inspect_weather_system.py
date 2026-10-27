"""Inspect the imported Fab weather Blueprint in Unreal without saving assets."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
out = root / 'Saved/WeatherSystem'
out.mkdir(parents=True, exist_ok=True)
cls = unreal.load_class(None, '/Game/WeatherSystem/BluePrints/WeatherManagmentBP.WeatherManagmentBP_C')
if not cls:
    raise RuntimeError('Weather Blueprint did not load')
cdo = unreal.get_default_object(cls)
data = {'class': cls.get_path_name(), 'members': {}}
for name in dir(cdo):
    if any(word in name.lower() for word in ('rain', 'humidity', 'weather', 'surface', 'initial')):
        try:
            value = getattr(cdo, name)
            data['members'][name] = {'value': str(value), 'doc': getattr(value, '__doc__', None)}
        except Exception as exc:
            data['members'][name] = str(exc)
data['components'] = [str(c) for c in cdo.get_components_by_class(unreal.ActorComponent)]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Maps/LV_BeachIsland_Weather')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
data['island_actors'] = [{'label': a.get_actor_label(), 'class': a.get_class().get_path_name(),
                          'location': str(a.get_actor_location())} for a in actors.get_all_level_actors()]
probe = actors.spawn_actor_from_class(cls, unreal.Vector())
data['probe'] = {}
for name in ('Rain', 'Humidity', 'RainOn?', 'RainEnabled?', 'RainSound', 'RainPreviousLocation'):
    try:
        data['probe'][name] = str(probe.get_editor_property(name))
    except Exception as exc:
        data['probe'][name] = str(exc)
data['probe_components'] = [str(c) for c in probe.get_components_by_class(unreal.ActorComponent)]
for name, args in [('SetRainComponentState', (True,)), ('SetRain', (0.5,)), ('SetHumidity', (0.5,))]:
    try:
        data['probe'][name] = str(probe.call_method(name, args))
    except Exception as exc:
        data['probe'][name] = str(exc)
rain = probe.get_component_by_class(unreal.NiagaraComponent)
data['probe']['spawn_multiplier'] = str(rain.get_variable_float('User.SpawnRateMultiplyer'))
data['niagara_members'] = [n for n in dir(unreal.NiagaraComponent) if 'variable' in n or 'system' in n]
(out / 'inspection.json').write_text(json.dumps(data, indent=2))
unreal.log('WEATHER_SYSTEM_INSPECTION_PASS')
