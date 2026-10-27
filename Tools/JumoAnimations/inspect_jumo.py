import unreal,json
from pathlib import Path
out={}
for path in ['/Game/Characters/BP_JumoCharacter','/Game/ThirdPerson/Blueprints/BP_JumoCharacter','/Game/Characters/jumo_final_glb_rigged/SkeletalMeshes/ABP_Jumo']:
 a=unreal.load_asset(path)
 if not a: continue
 out[path]={'class':a.get_class().get_name(),'dependencies':list(map(str,unreal.AssetRegistryHelpers.get_asset_registry().get_dependencies(unreal.Name(path),unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True))))}
 if isinstance(a,unreal.Blueprint):
  c=unreal.get_default_object(a.generated_class()); out[path]['parent']=str(a.parent_class) if hasattr(a,'parent_class') else str(c.get_class())
  if isinstance(c,unreal.Character):
   m=c.get_editor_property('mesh'); out[path]['mesh']=str(m.get_editor_property('skeletal_mesh_asset')); out[path]['anim']=str(m.get_editor_property('anim_class')); out[path]['speed']=c.get_editor_property('character_movement').get_editor_property('max_walk_speed')
Path(unreal.Paths.project_dir(),'Saved/JumoAnimations/inspect.json').write_text(json.dumps(out,indent=2))
unreal.log('JUMO_INSPECT_PASS')
