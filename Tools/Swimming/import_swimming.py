"""Unreal Python: import only the new swimming animations onto the original rig."""
import unreal
import hashlib
import json
from pathlib import Path

root = Path(unreal.Paths.project_dir()).resolve()
skeleton = unreal.load_asset('/Game/Characters/jumo_final_glb_rigged/SkeletalMeshes/jumo_final_glb_rigged_Skeleton')
assert isinstance(skeleton,unreal.Skeleton)
originals = list((root/'Content/Characters/jumo_final_glb_rigged').rglob('*.uasset'))
before = {p:hashlib.sha256(p.read_bytes()).hexdigest() for p in originals}
report=[]
for label in ['SwimIdle','SwimForward']:
    destination='/Game/Characters/JumoAnimations/Swimming'
    target=destination+'/AS_Fairy_'+label
    if not unreal.EditorAssetLibrary.does_asset_exist(target):
        pipeline=unreal.InterchangeGenericAssetsPipeline()
        common=pipeline.get_editor_property('common_skeletal_meshes_and_animations_properties')
        common.set_editor_property('skeleton',skeleton)
        common.set_editor_property('import_only_animations',True)
        pipeline.get_editor_property('mesh_pipeline').set_editor_property('import_skeletal_meshes',True)
        pipeline.get_editor_property('mesh_pipeline').set_editor_property('import_static_meshes',False)
        pipeline.get_editor_property('animation_pipeline').set_editor_property('import_animations',True)
        params=unreal.ImportAssetParameters()
        params.set_editor_property('is_automated',True)
        params.set_editor_property('replace_existing',False)
        params.set_editor_property('override_pipelines',[unreal.SoftObjectPath(pipeline.get_path_name())])
        # Separate destinations avoid collisions and identify the single clip
        # even when Interchange includes the source rig's name in the asset name.
        staging=destination+'/Import_'+label
        source=unreal.InterchangeManager.create_source_data(str(root/'Saved/Swimming/Retargeted'/('AS_Fairy_'+label+'.glb')))
        if not unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(staging,source,params):
            raise RuntimeError('Swimming import failed: '+label)
        candidates=[unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(staging,recursive=True)]
        clips=[a for a in candidates if isinstance(a,unreal.AnimSequence)]
        if len(clips)!=1: raise RuntimeError('Expected one swimming clip: '+str(clips))
        if not unreal.EditorAssetLibrary.rename_asset(clips[0].get_path_name(),target):
            raise RuntimeError('Rename failed: '+target)
    clip=unreal.load_asset(target)
    assert isinstance(clip,unreal.AnimSequence) and clip.get_play_length()>0
    assert clip.get_editor_property('skeleton')==skeleton
    # Hips translation is already in-place in the bake. Do not lock the root's
    # rotation: horizontal swimming needs the authored body pitch.
    clip.set_editor_property('enable_root_motion',False)
    clip.set_editor_property('force_root_lock',False)
    assert unreal.EditorAssetLibrary.save_loaded_asset(clip)
    report.append({'path':target,'length':clip.get_play_length(),'skeleton':skeleton.get_path_name()})
for path,digest in before.items():
    if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise RuntimeError('Original asset changed: '+str(path))
(root/'Saved/Swimming/import_report.json').write_text(json.dumps(report,indent=2))
unreal.log('FAIRY_SWIMMING_IMPORT_PASS: '+json.dumps(report))
