"""Blender: export the game's original fairy for Mixamo without editing it."""
import bpy
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = root / 'Source/glb/jumo_final.glb_rigged.glb'
output = root / 'Saved/Swimming/Source'
output.mkdir(parents=True, exist_ok=True)
before = hashlib.sha256(source.read_bytes()).hexdigest()
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
rig.animation_data_clear()
for bone in rig.pose.bones:
    bone.matrix_basis.identity()
rig.data.pose_position = 'REST'
for obj in bpy.context.scene.objects:
    if obj.type == 'MESH':
        obj.animation_data_clear()
mapping = {'hips':'Hips', 'spine':'Spine', 'chest':'Spine1', 'neck':'Neck', 'head':'Head',
           'shoulder.L':'LeftShoulder', 'upperArm.L':'LeftArm', 'lowerArm.L':'LeftForeArm', 'hand.L':'LeftHand',
           'shoulder.R':'RightShoulder', 'upperArm.R':'RightArm', 'lowerArm.R':'RightForeArm', 'hand.R':'RightHand',
           'upperLeg.L':'LeftUpLeg', 'lowerLeg.L':'LeftLeg', 'foot.L':'LeftFoot',
           'upperLeg.R':'RightUpLeg', 'lowerLeg.R':'RightLeg', 'foot.R':'RightFoot'}
for old, new in mapping.items():
    rig.data.bones[old].name = 'mixamorig:' + new
bpy.ops.object.select_all(action='SELECT')
fbx = output / 'Fairy_CurrentGame_Mixamo.fbx'
bpy.ops.export_scene.fbx(filepath=str(fbx), object_types={'MESH','ARMATURE'},
    add_leaf_bones=False, bake_anim=False, path_mode='COPY', embed_textures=True,
    axis_forward='-Z', axis_up='Y')
assert hashlib.sha256(source.read_bytes()).hexdigest() == before
(output / 'manifest.json').write_text(json.dumps({'original':str(source.relative_to(root)),
    'sha256':before,'fbx':str(fbx.relative_to(root)), 'bones':mapping},indent=2))
print('FAIRY_MIXAMO_EXPORT_PASS', fbx)
