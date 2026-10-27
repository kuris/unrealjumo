import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/Maps/LV_BeachIsland_Weather'): raise RuntimeError('Load failed')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
ignore=[a for a in actors if a.get_class().get_name()!='Landscape']
samples=[]
for y in range(-20000,20001,1000):
 for x in range(-20000,20001,1000):
  hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,3000),unreal.Vector(x,y,-3000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,unreal.DrawDebugTrace.NONE,True)
  if hit:
   info=hit.to_tuple()
   p=info[5]; n=info[7]
   samples.append([x,y,p.z,n.z])
root=Path(unreal.Paths.project_dir()).resolve()
(root/'Saved/JumoAnimations/island_heights.json').write_text(json.dumps(samples))
unreal.log('ISLAND_SAMPLE_PASS: '+str(len(samples)))
for name in ['seonbi','pojol']:
 mesh=unreal.load_asset('/Game/Characters/'+name+'_final_rigged/'+name+'_final_rigged')
 bounds=mesh.get_bounds()
 unreal.log('MESH_BOUNDS '+name+' '+str(bounds))
