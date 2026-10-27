"""Import all seven source clips onto the existing Jumo skeleton (Unreal Python)."""
import hashlib
import json
from pathlib import Path
import unreal


def main():
    root = Path(unreal.Paths.project_dir())
    source_path = root / 'Source/glb/jumo_final.glb_rigged.glb'
    if not source_path.is_file():
        raise RuntimeError('Source not found: ' + str(source_path))
    destination = '/Game/Characters/JumoAnimations'
    skeleton = unreal.load_asset('/Game/Characters/jumo_final_glb_rigged/SkeletalMeshes/jumo_final_glb_rigged_Skeleton')
    if not isinstance(skeleton, unreal.Skeleton):
        raise RuntimeError('Existing Jumo skeleton not found')
    originals = list((root / 'Content/Characters/jumo_final_glb_rigged').rglob('*.uasset'))
    before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in originals}
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    common = pipeline.get_editor_property('common_skeletal_meshes_and_animations_properties')
    common.set_editor_property('skeleton', skeleton)
    common.set_editor_property('import_only_animations', True)
    # Skeletal factory nodes must exist for animation binding. Animation-only
    # mode disables saving the skeleton and mesh later in this pipeline.
    pipeline.get_editor_property('mesh_pipeline').set_editor_property('import_skeletal_meshes', True)
    pipeline.get_editor_property('mesh_pipeline').set_editor_property('import_static_meshes', False)
    pipeline.get_editor_property('animation_pipeline').set_editor_property('import_animations', True)
    params = unreal.ImportAssetParameters()
    params.set_editor_property('is_automated', True)
    params.set_editor_property('replace_existing', False)
    params.set_editor_property('override_pipelines', [unreal.SoftObjectPath(pipeline.get_path_name())])
    assets = unreal.EditorAssetLibrary.list_assets(destination, recursive=True) if unreal.EditorAssetLibrary.does_directory_exist(destination) else []
    if not assets:
        source = unreal.InterchangeManager.create_source_data(str(source_path))
        if not unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(destination, source, params):
            raise RuntimeError('Jumo animation import failed')
    report = []
    for name in unreal.EditorAssetLibrary.list_assets(destination, recursive=True):
        asset = unreal.load_asset(name)
        if isinstance(asset, unreal.AnimSequence):
            if asset.get_editor_property('skeleton') != skeleton or asset.get_play_length() <= 0:
                raise RuntimeError('Invalid animation: ' + name)
            if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
                raise RuntimeError('Save failed: ' + name)
            report.append({'path': asset.get_path_name(), 'name': asset.get_name(), 'length': asset.get_play_length(), 'skeleton': skeleton.get_path_name()})
    if len(report) != 7:
        raise RuntimeError('Expected seven animations: ' + str(report))
    for path, digest in before.items():
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise RuntimeError('Original Jumo asset changed: ' + str(path))
    output = root / 'Saved/JumoAnimations'
    output.mkdir(parents=True, exist_ok=True)
    (output / 'import.json').write_text(json.dumps(report, indent=2))
    unreal.log('JUMO_IMPORT_PASS: seven animations; original Jumo assets unchanged')


if __name__ == '__main__':
    main()
