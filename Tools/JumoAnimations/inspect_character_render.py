import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
report={}
for name in ['seonbi','pojol']:
 prefix='/Game/Characters/'+name+'_final_rigged/'+name+'_final_rigged'
 mesh=unreal.load_asset(prefix)
 material=unreal.load_asset(prefix+'_material_0')
 row={'material_class':material.get_class().get_name(),'materials':str(mesh.get_editor_property('materials')),'bounds':str(mesh.get_bounds())}
 lib=unreal.MaterialEditingLibrary
 if isinstance(material,unreal.MaterialInstanceConstant):
  row['parent']=str(material.get_editor_property('parent'))
  row['textures']={str(n):str(lib.get_material_instance_texture_parameter_value(material,n)) for n in lib.get_texture_parameter_names(material)}
  row['scalars']={str(n):lib.get_material_instance_scalar_parameter_value(material,n) for n in lib.get_scalar_parameter_names(material)}
  row['switches']={str(n):lib.get_material_instance_static_switch_parameter_value(material,n) for n in lib.get_static_switch_parameter_names(material)}
 row['animations']={}
 for clip in ['idle','walk']:
  a=unreal.load_asset(prefix+clip)
  row['animations'][clip]={n:str(a.get_editor_property(n)) for n in ['enable_root_motion','force_root_lock','root_motion_root_lock']}
 report[name]=row
unreal.log('POSE_EXTENSIONS: '+str(dir(unreal.AnimPoseExtensions)))
unreal.log('POSE_GET_DOC: '+str(unreal.AnimPoseExtensions.get_anim_pose_at_time.__doc__))
unreal.log('POSE_BONE_DOC: '+str(unreal.AnimPoseExtensions.get_bone_pose.__doc__))
(root/'Saved/JumoAnimations/character_render_inspect.json').write_text(json.dumps(report,indent=2))
unreal.log('CHARACTER_RENDER_INSPECT_PASS')
