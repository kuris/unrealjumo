import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/Maps/LV_BeachIsland_Weather'): raise RuntimeError('Load failed')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
def vec(v): return [v.x,v.y,v.z]
report=[]
for a in actors:
 origin,extent=a.get_actor_bounds(False)
 row={'label':a.get_actor_label(),'class':a.get_class().get_name(),'location':vec(a.get_actor_location()),'rotation':str(a.get_actor_rotation()),'scale':vec(a.get_actor_scale3d()),'bounds_origin':vec(origin),'bounds_extent':vec(extent)}
 row['meshes']=[str(c.get_editor_property('static_mesh')) for c in a.get_components_by_class(unreal.StaticMeshComponent)]
 report.append(row)
root=Path(unreal.Paths.project_dir()).resolve()
(root/'Saved/JumoAnimations/island_inspect.json').write_text(json.dumps(report,indent=2))
unreal.log('TRACE_DOC: '+str(unreal.SystemLibrary.line_trace_single.__doc__))
unreal.log('HIT_DOC: '+str(unreal.HitResult.__doc__))
unreal.log('BOUNDS_DOC: '+str(unreal.SkeletalMesh.get_bounds.__doc__))
unreal.log('ISLAND_INSPECT_PASS')
