"""Blender: transfer Mixamo motions from the uploaded fairy onto its original rig.

Mixamo preserves the uploaded nineteen-joint rig. Transfer evaluated world
rotation with rest-frame correction; retain original proportions and names.
Only new GLB animation sources are written; the game character stays untouched.
"""
import bpy
import hashlib
import json
import math
from pathlib import Path
from mathutils import Matrix

root = Path(__file__).resolve().parents[2]
source = root / 'Source/glb/jumo_final.glb_rigged.glb'
out = root / 'Saved/Swimming/Retargeted'
out.mkdir(parents=True, exist_ok=True)
mapping = json.loads((root/'Saved/Swimming/Source/manifest.json').read_text())['bones']
report = []
for label, filename in [('SwimIdle','Treading Water.fbx'),('SwimForward','Swimming.fbx')]:
    donor_file = root/'Saved/Swimming/Mixamo'/filename
    if not donor_file.exists():
        raise RuntimeError('Missing Mixamo download: '+str(donor_file))
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    target = next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    target.animation_data_clear()
    for p in target.pose.bones: p.matrix_basis = Matrix.Identity(4)
    for action in list(bpy.data.actions): bpy.data.actions.remove(action)
    before = set(bpy.context.scene.objects)
    bpy.ops.import_scene.fbx(filepath=str(donor_file))
    incoming = [o for o in bpy.context.scene.objects if o not in before]
    donor = next(o for o in incoming if o.type=='ARMATURE')
    action = donor.animation_data.action
    first, last = map(int, action.frame_range)
    donor_map = {b.name.split(':')[-1]:b for b in donor.pose.bones}
    missing = [n for n in mapping.values() if n not in donor_map]
    if missing: raise RuntimeError('Missing donor bones: '+str(missing))
    clip = bpy.data.actions.new('AS_Fairy_'+label)
    target.animation_data_create()
    target.animation_data.action = clip
    target.animation_data.action_slot = clip.slots.new(id_type='OBJECT',name=target.name)
    target_world = target.matrix_world.to_quaternion()
    donor_world = donor.matrix_world.to_quaternion()
    changes = 0.
    for frame in range(first, last+1):
        bpy.context.scene.frame_set(frame)
        desired = {}
        for p in target.pose.bones:
            s = donor_map[mapping[p.name]]
            parent = desired[p.parent.name] if p.parent else Matrix.Identity(4)
            rest = p.parent.bone.matrix_local.inverted() @ p.bone.matrix_local if p.parent else p.bone.matrix_local
            # Same uploaded character: preserve the world-space animation delta
            # and put its axes back in the unmodified target's rest frame.
            donor_rest = donor_world @ s.bone.matrix_local.to_quaternion()
            target_rest = target_world @ p.bone.matrix_local.to_quaternion()
            correction = donor_rest.inverted() @ target_rest
            rotation = target_world.inverted() @ donor_world @ s.matrix.to_quaternion() @ correction
            position = (parent @ rest).translation if p.parent else p.bone.matrix_local.translation.copy()
            desired[p.name] = Matrix.Translation(position) @ rotation.to_matrix().to_4x4()
            p.rotation_mode = 'QUATERNION'
            p.matrix_basis = rest.inverted() @ parent.inverted() @ desired[p.name]
            p.keyframe_insert(data_path='rotation_quaternion',frame=frame-first)
            p.keyframe_insert(data_path='location',frame=frame-first)
        bpy.context.view_layer.update()
    for obj in incoming: bpy.data.objects.remove(obj,do_unlink=True)
    for old in list(bpy.data.actions):
        if old != clip: bpy.data.actions.remove(old)
    bpy.context.scene.frame_start = 0
    bpy.context.scene.frame_end = last-first
    bpy.context.scene.render.fps = 30
    bpy.ops.object.select_all(action='DESELECT')
    target.select_set(True)
    bpy.context.view_layer.objects.active = target
    destination = out/('AS_Fairy_'+label+'.glb')
    # Skin nodes establish animation binding for Unreal's animation-only import.
    # Meshes are export sources only and are never imported into the game.
    for obj in bpy.context.scene.objects:
        if obj.type=='MESH': obj.select_set(True)
    bpy.ops.export_scene.gltf(filepath=str(destination),export_format='GLB',use_selection=True,
        export_animations=True,export_frame_range=True,export_force_sampling=True,
        export_nla_strips=False,export_skins=True,export_materials='NONE')
    report.append({'clip':label,'frames':last-first+1,'fps':30,'source':filename,
        'source_sha256':hashlib.sha256(donor_file.read_bytes()).hexdigest(),
        'retargeted':str(destination.relative_to(root)), 'bones':len(mapping)})
(out/'report.json').write_text(json.dumps(report,indent=2))
print('FAIRY_SWIMMING_RETARGET_PASS',json.dumps(report))
