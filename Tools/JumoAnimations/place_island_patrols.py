"""Place Seonbi and Pojol on ground-checked loops around the island hill."""
import hashlib
import json
import math
import shutil
from datetime import datetime
from pathlib import Path
import unreal

MAP = '/Game/Maps/LV_BeachIsland_Weather'


def main():
    root = Path(unreal.Paths.project_dir()).resolve()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if editor.get_game_world() or unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages() or unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError('Save changes and stop Play before placing patrols')
    baseline = json.loads((root/'Tools/WeatherTest/protected_baseline.json').read_text())
    def verify():
        for name, expected in baseline.items():
            if hashlib.sha256((root/name).read_bytes()).hexdigest() != expected:
                raise RuntimeError('Protected file differs: '+name)
    verify()
    if not levels.load_level(MAP):
        raise RuntimeError('Could not load island weather map')
    world = editor.get_editor_world()
    if world.get_path_name() != MAP+'.LV_BeachIsland_Weather':
        raise RuntimeError('Unexpected world')
    original_actors = list(actors.get_all_level_actors())
    if any(a.get_level() != levels.get_current_level() for a in original_actors):
        raise RuntimeError('Shared sublevels are not supported')
    ignore_ground = [a for a in original_actors if a.get_class().get_name() != 'Landscape']
    def ground(x,y):
        hit = unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,3000),unreal.Vector(x,y,-3000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore_ground,unreal.DrawDebugTrace.NONE,True)
        if hit is None:
            raise RuntimeError('Missing ground: '+str((x,y)))
        info=hit.to_tuple()
        point,normal=info[5],info[7]
        if point.z < 40 or normal.z < 0.8:
            raise RuntimeError('Unsafe patrol ground: '+str((x,y,point.z,normal.z)))
        return point
    # The sampled northern mound rises to ~5 m; these loops surround its foot.
    center=(900.,700.)
    specifications=[('seonbi',2900.,3300.,math.pi,1,155.),('pojol',3150.,3550.,math.pi+0.12,-1,145.)]
    plans=[]
    for name,rx,ry,phase,direction,speed in specifications:
        points=[]
        for i in range(96):
            angle=phase+direction*2*math.pi*i/96
            x=center[0]+rx*math.cos(angle); y=center[1]+ry*math.sin(angle)
            point=ground(x,y)
            # Check the width of a character's capsule, as well as its center.
            for dx,dy in [(40,0),(-40,0),(0,40),(0,-40)]:
                side=ground(x+dx,y+dy)
                if abs(side.z-point.z)>40:
                    raise RuntimeError('Ground edge near patrol route')
            points.append(point)
        mesh_path='/Game/Characters/'+name+'_final_rigged/'+name+'_final_rigged'
        mesh=unreal.load_asset(mesh_path)
        walk=unreal.load_asset(mesh_path+'walk')
        idle=unreal.load_asset(mesh_path+'idle')
        if not isinstance(mesh,unreal.SkeletalMesh) or not isinstance(walk,unreal.AnimSequence) or not isinstance(idle,unreal.AnimSequence):
            raise RuntimeError('Missing imported character: '+name)
        plans.append((name,points,mesh,walk,idle,speed))
    patrol_class=unreal.load_class(None,'/Script/SimpleWeather.IslandPatrolCharacter')
    if not patrol_class:
        raise RuntimeError('Compile IslandPatrolCharacter before running placement')
    backups=root/'Saved/JumoAnimations/MapBackups'
    backups.mkdir(parents=True,exist_ok=True)
    shutil.copy2(root/'Content/Maps/LV_BeachIsland_Weather.umap',backups/('LV_BeachIsland_Weather_'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.umap'))
    report=[]
    with unreal.ScopedEditorTransaction('Place two hill patrol characters'):
        for name,points,mesh,walk,idle,speed in plans:
            label='Hill Patrol - '+name.capitalize()
            matches=[a for a in original_actors if a.get_actor_label()==label]
            if len(matches)>1 or (matches and matches[0].get_class()!=patrol_class):
                raise RuntimeError('Unexpected actor at '+label)
            location=unreal.Vector(points[0].x,points[0].y,points[0].z+90.)
            actor=matches[0] if matches else actors.spawn_actor_from_class(patrol_class,location)
            actor.modify()
            actor.set_actor_label(label)
            actor.set_folder_path('Island/Hill Patrols')
            actor.set_actor_location(location,False,True)
            component=actor.get_editor_property('mesh')
            component.set_skeletal_mesh_asset(mesh)
            material=unreal.load_asset('/Game/Characters/'+name+'_final_rigged/Mobile/M_'+name+'_Mobile')
            if not material: raise RuntimeError('Missing mobile material: '+name)
            component.set_material(0,material)
            component.set_relative_location(unreal.Vector(0,0,-88),False,False)
            component.set_relative_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False,False)
            component.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
            animation_data=component.get_editor_property('animation_data')
            animation_data.set_editor_property('anim_to_play',idle)
            animation_data.set_editor_property('saved_looping',True)
            animation_data.set_editor_property('saved_playing',True)
            component.set_editor_property('animation_data',animation_data)
            actor.set_editor_property('patrol_name',name)
            actor.set_editor_property('patrol_points',points)
            actor.set_editor_property('walk_animation',walk)
            actor.set_editor_property('idle_animation',idle)
            actor.set_editor_property('walk_speed',speed)
            tangent=points[1]-points[0]
            actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=math.degrees(math.atan2(tangent.y,tangent.x)),roll=0),False)
            report.append({'name':name,'actor':actor.get_path_name(),'points':[[p.x,p.y,p.z] for p in points], 'speed_cm_s':speed,'direction': 'counterclockwise' if name=='seonbi' else 'clockwise'})
    # Start this opt-in test copy beside the patrols so both are visible on launch.
    start=next((a for a in original_actors if isinstance(a,unreal.PlayerStart)),None)
    if start:
        start.modify()
        start.set_actor_location(unreal.Vector(-2500,1050,200),False,True)
        start.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-20,roll=0),False)
    if not levels.save_current_level():
        raise RuntimeError('Could not save patrol map')
    verify()
    (root/'Saved/JumoAnimations/patrol_placement.json').write_text(json.dumps({'map':MAP,'hill_center':center,'patrols':report,'ground_checks':'96 points per character plus capsule margins; dry ground, normal z >= 0.8','device_validation':'pending'},indent=2))
    unreal.log('ISLAND_PATROL_PLACEMENT_PASS: two characters, 96 safe waypoints each; original maps unchanged')


if __name__=='__main__':
    main()
