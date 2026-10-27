"""Import the remaining Source/glb characters into isolated Content folders.

Run with UnrealEditor -run=pythonscript -script=<this file>.
Existing destinations are validated instead of overwritten.
"""
import hashlib
import json
import struct
from pathlib import Path
import unreal


def read_glb(path):
    with path.open('rb') as stream:
        magic, version, _ = struct.unpack('<III', stream.read(12))
        length, kind = struct.unpack('<II', stream.read(8))
        if magic != 0x46546C67 or version != 2 or kind != 0x4E4F534A:
            raise RuntimeError('Invalid GLB: ' + str(path))
        return json.loads(stream.read(length))


def import_character(source, destination):
    document = read_glb(source)
    expected_clips = [clip['name'] for clip in document.get('animations', [])]
    existing = unreal.EditorAssetLibrary.list_assets(destination, recursive=True) if unreal.EditorAssetLibrary.does_directory_exist(destination) else []
    if not existing:
        pipeline = unreal.InterchangeGenericAssetsPipeline()
        common = pipeline.get_editor_property('common_skeletal_meshes_and_animations_properties')
        common.set_editor_property('import_only_animations', False)
        common.set_editor_property('try_auto_select_skeleton', False)
        pipeline.get_editor_property('mesh_pipeline').set_editor_property('import_skeletal_meshes', True)
        pipeline.get_editor_property('mesh_pipeline').set_editor_property('import_static_meshes', False)
        pipeline.get_editor_property('animation_pipeline').set_editor_property('import_animations', True)
        params = unreal.ImportAssetParameters()
        params.set_editor_property('is_automated', True)
        params.set_editor_property('replace_existing', False)
        params.set_editor_property('override_pipelines', [unreal.SoftObjectPath(pipeline.get_path_name())])
        data = unreal.InterchangeManager.create_source_data(str(source))
        if not unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(destination, data, params):
            raise RuntimeError('Import failed: ' + str(source))
    assets = [unreal.load_asset(name) for name in unreal.EditorAssetLibrary.list_assets(destination, recursive=True)]
    meshes = [a for a in assets if isinstance(a, unreal.SkeletalMesh)]
    clips = [a for a in assets if isinstance(a, unreal.AnimSequence)]
    materials = [a for a in assets if isinstance(a, unreal.MaterialInterface)]
    textures = [a for a in assets if isinstance(a, unreal.Texture)]
    if len(meshes) != 1 or len(clips) != len(expected_clips) or not materials or not textures:
        raise RuntimeError('Incomplete character import: ' + destination + ' ' + str([(a.get_name(), a.get_class().get_name()) for a in assets]))
    skeleton = meshes[0].get_editor_property('skeleton')
    if not skeleton or not skeleton.get_path_name().startswith(destination + '/'):
        raise RuntimeError('Character must have its own skeleton: ' + destination)
    for clip in clips:
        if clip.get_editor_property('skeleton') != skeleton or clip.get_play_length() <= 0:
            raise RuntimeError('Invalid clip: ' + clip.get_path_name())
    for expected in expected_clips:
        # Interchange prefixes source names and sanitizes '-' to '_'.
        if not any(clip.get_name().lower().replace('-', '_').endswith(expected.replace('-', '_')) for clip in clips):
            raise RuntimeError('Missing clip: ' + expected)
    for asset in assets:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
            raise RuntimeError('Save failed: ' + asset.get_path_name())
    report = {
        'source': str(source), 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'destination': destination, 'mesh': meshes[0].get_path_name(), 'skeleton': skeleton.get_path_name(),
        'animations': [{'path': a.get_path_name(), 'seconds': a.get_play_length()} for a in clips],
        'materials': [a.get_path_name() for a in materials], 'textures': [a.get_path_name() for a in textures],
    }
    unreal.log('GLB_CHARACTER_PASS: ' + destination + ', ' + str(len(clips)) + ' animations')
    return report


def main():
    root = Path(unreal.Paths.project_dir())
    sources = sorted((root / 'Source/glb').glob('*.glb'))
    # Jumo already exists; its seven clips are imported by import_jumo.py.
    sources = [source for source in sources if not source.name.startswith('jumo_')]
    if not sources:
        raise RuntimeError('No character GLBs found')
    existing_files = list((root / 'Content').rglob('*.uasset')) + list((root / 'Content').rglob('*.umap'))
    before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in existing_files}
    reports = []
    for source in sources:
        destination = '/Game/Characters/' + source.stem
        reports.append(import_character(source, destination))
    for path, digest in before.items():
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise RuntimeError('Preexisting content changed: ' + str(path))
    output = root / 'Saved/JumoAnimations'
    output.mkdir(parents=True, exist_ok=True)
    (output / 'characters_import.json').write_text(json.dumps(reports, indent=2))
    unreal.log('GLB_CHARACTERS_IMPORT_PASS: existing content unchanged')


if __name__ == '__main__':
    main()
