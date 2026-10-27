import unreal
animation=unreal.load_asset('/Game/Characters/JumoAnimations/jumo_final_glb_riggedjump')
if not isinstance(animation,unreal.AnimSequence):raise RuntimeError('Jump animation not found')
# CharacterMovement supplies the jump arc. Keep the clip from adding a second
# vertical displacement through its hips root track.
animation.set_editor_property('force_root_lock',True)
animation.set_editor_property('root_motion_root_lock',unreal.RootMotionRootLock.REF_POSE)
if not unreal.EditorAssetLibrary.save_loaded_asset(animation):raise RuntimeError('Save failed')
unreal.log('JUMO_JUMP_ROOT_PASS')
