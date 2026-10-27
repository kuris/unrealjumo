"""Build and archive a standalone Mac Development game for island validation."""
import os
from pathlib import Path
import subprocess
import sys
root=Path(__file__).resolve().parents[2]
engine=Path('/Users/Shared/Epic Games/UE_5.8/Engine')
subprocess.run([sys.executable,str(root/'Tools/WeatherTest/verify_protected.py')],check=True)
# Keep the cooker's reflected class layout in sync with the packaged game.
subprocess.run([sys.executable,str(root/'Tools/WeatherTest/build_and_generate.py'),
 '--engine',str(engine.parent),'--platform','Mac','--build-only'],check=True)
output=root/'Saved/IslandMacPackage'
logs=root/'Saved/IslandMacLogs'
logs.mkdir(parents=True,exist_ok=True)
environment=os.environ.copy()
environment['uebp_LogFolder']=str(logs)
environment['uebp_FinalLogFolder']=str(logs)
command=[str(engine/'Build/BatchFiles/RunUAT.sh'),'BuildCookRun',f'-project={root / "buildtest.uproject"}','-target=buildtest',f'-unrealexe={engine / "Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"}','-platform=Mac','-clientconfig=Development','-build','-cook','-stage','-package','-archive','-pak','-iostore','-compressed','-nop4','-utf8output','-unattended','-skipbuildeditor','-nocompileeditor','-ubtargs=-NoUBA -UBANoDetour -MaxParallelActions=2','-map=/Game/Maps/LV_BeachIsland_Weather',f'-cookdir={root / "Content/Weather"}',f'-cookdir={root / "Content/Characters/JumoAnimations"}','-cmdline=/Game/Maps/LV_BeachIsland_Weather',f'-archivedirectory={output}']
with (root/'Saved/JumoAnimations/mac_package.log').open('w') as log:
 result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,env=environment)
subprocess.run([sys.executable,str(root/'Tools/WeatherTest/verify_protected.py')],check=True)
if result.returncode:raise SystemExit(result.returncode)
print('ISLAND_MAC_PACKAGE_PASS:',output,flush=True)
