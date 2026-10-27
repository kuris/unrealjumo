import unreal,json
from pathlib import Path
unreal.log('ROTATOR_POSITIONAL: '+str(unreal.Rotator(0,-90,0)))
unreal.log('ROTATOR_KEYWORD: '+str(unreal.Rotator(pitch=0,yaw=-90,roll=0)))
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Maps/LV_BeachIsland_Weather')
rows=[]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if a.get_class().get_name()!='IslandPatrolCharacter':continue
 m=a.get_editor_property('mesh')
 rows.append({'label':a.get_actor_label(),'actor_rotation':str(a.get_actor_rotation()),'mesh_rotation':str(m.get_editor_property('relative_rotation')),'mesh_world_rotation':str(m.get_world_rotation()),'mesh_bounds':str(m.get_editor_property('skeletal_mesh_asset').get_bounds())})
Path(unreal.Paths.project_dir()).resolve().joinpath('Saved/JumoAnimations/patrol_rotation.json').write_text(json.dumps(rows,indent=2))
unreal.log('PATROL_ROTATION_INSPECT_PASS')
