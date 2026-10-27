import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Maps/LV_BeachIsland_Weather')
rows=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if actor.get_class().get_name()!='IslandPatrolCharacter':continue
 mesh=actor.get_editor_property('mesh')
 asset=mesh.get_editor_property('skeletal_mesh_asset')
 row={'name':actor.get_actor_label(),'component_material':str(mesh.get_material(0)),'override_materials':str(mesh.get_editor_property('override_materials')),'asset_materials':str(asset.get_editor_property('materials'))}
 rows.append(row)
for name in ['seonbi','pojol']:
 path='/Game/Characters/'+name+'_final_rigged/Mobile/M_'+name+'_Mobile'
 material=unreal.load_asset(path)
 rows.append({'name':name,'material':str(material),'skeletal_usage':material.get_editor_property('used_with_skeletal_mesh'),'base_color_input':str(unreal.MaterialEditingLibrary.get_material_property_input_node(material,unreal.MaterialProperty.MP_BASE_COLOR)),'texture_nodes':[str(e.get_editor_property('texture')) for e in unreal.MaterialEditingLibrary.get_material_expressions(material) if isinstance(e,unreal.MaterialExpressionTextureSample)]})
root=Path(unreal.Paths.project_dir()).resolve()
(root/'Saved/JumoAnimations/saved_materials_inspect.json').write_text(json.dumps(rows,indent=2))
unreal.log('SAVED_MATERIALS_INSPECT_PASS')
