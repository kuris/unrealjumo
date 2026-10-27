"""Import the project's original synthesized wind and surf loops (Unreal Python)."""
import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
report=[]
for name in ['Wind','Surf']:
 path='/Game/Weather/Audio/S_'+name
 sound=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
 if sound is None:
  task=unreal.AssetImportTask()
  task.set_editor_properties({'filename':str(root/'Source/Audio'/('Island_'+name+'.wav')),'destination_path':'/Game/Weather/Audio','destination_name':'S_'+name,'automated':True,'save':True})
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  sound=unreal.load_asset(path)
 if not isinstance(sound,unreal.SoundWave):raise RuntimeError('Sound import failed: '+name)
 sound.modify()
 sound.set_editor_property('looping',True)
 if not unreal.EditorAssetLibrary.save_loaded_asset(sound,False):raise RuntimeError('Sound save failed')
 duration=sound.get_editor_property('duration')
 if duration<24:raise RuntimeError('Ambient loop too short')
 report.append({'asset':sound.get_path_name(),'seconds':duration,'looping':True})
(root/'Saved/JumoAnimations/ambience.json').write_text(json.dumps(report,indent=2))
unreal.log('ISLAND_AMBIENCE_IMPORT_PASS')
