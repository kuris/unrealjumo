import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
report={}
for name in ['seonbi','pojol']:
 prefix='/Game/Characters/'+name+'_final_rigged/'+name+'_final_rigged'
 m=unreal.load_asset(prefix+'_material_0')
 lib=unreal.MaterialEditingLibrary
 row={'material_inputs':{},'expressions':[],'poses':{}}
 for prop in [unreal.MaterialProperty.MP_BASE_COLOR,unreal.MaterialProperty.MP_NORMAL,unreal.MaterialProperty.MP_METALLIC,unreal.MaterialProperty.MP_ROUGHNESS]:
  node=lib.get_material_property_input_node(m,prop)
  row['material_inputs'][str(prop)]=str(node)
 for e in lib.get_material_expressions(m):
  info={'class':e.get_class().get_name(),'name':e.get_name()}
  if isinstance(e,unreal.MaterialExpressionTextureSample):
   info['texture']=str(e.get_editor_property('texture'))
  row['expressions'].append(info)
 for clip in ['idle','walk']:
  a=unreal.load_asset(prefix+clip)
  samples=[]
  for t in [0,a.get_play_length()*0.25,a.get_play_length()*0.5,a.get_play_length()*0.75]:
   pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,t,unreal.AnimPoseEvaluationOptions())
   bones={}
   for bone in unreal.AnimPoseExtensions.get_bone_names(pose):
    transform=unreal.AnimPoseExtensions.get_bone_pose(pose,bone,unreal.AnimPoseSpaces.WORLD)
    bones[str(bone)]=str(transform)
   samples.append({'time':t,'bones':bones})
  row['poses'][clip]=samples
 report[name]=row
(root/'Saved/JumoAnimations/character_pose_inspect.json').write_text(json.dumps(report,indent=2))
unreal.log('CHARACTER_POSE_INSPECT_PASS')
