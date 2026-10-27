"""Run the packaged Mac game, record the actual rendering and validate its report."""
from datetime import datetime
import json
from pathlib import Path
import shutil
import subprocess
import time

root=Path(__file__).resolve().parents[2]
app=root/'Saved/IslandMacPackage/Mac/buildtest.app'
output=root/'Saved/IslandMacValidation'/datetime.now().strftime('%Y%m%d-%H%M%S')
output.mkdir(parents=True)
captures=Path.home()/'Library/Containers/com.YourCompany.buildtest/Data/Library/Caches/IslandValidation'/output.name
captures.mkdir(parents=True,exist_ok=True)
log_path=output/'runtime.log'
command=[str(app/'Contents/MacOS/buildtest'),'/Game/Maps/LV_BeachIsland_Weather','-windowed','-ResX=1280','-ResY=800','-ForceRes','-NoSplash','-IslandValidate','-DialogueValidate','-IslandValidationExit','-IslandValidationOutput='+str(captures),'-stdout','-forcelogflush']
with log_path.open('w') as log:
 process=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT)
 print('MAC_VALIDATION_STARTED:',process.pid,output,flush=True)
 deadline=time.monotonic()+420
 last_report=time.monotonic()
 activated=False
 while process.poll() is None:
  data=log_path.read_text(errors='replace')
  if not activated and 'Engine is initialized' in data:
   subprocess.run(['osascript','-e','tell application "'+str(app)+'" to activate'],capture_output=True)
   activated=True
  if time.monotonic()-last_report>20:
   rows=[line for line in data.splitlines() if 'ISLAND_CHECK_' in line or 'ISLAND_PATROL_PROGRESS' in line or 'ISLAND_CAPTURE_SAVED' in line]
   print('\n'.join(rows[-3:]),flush=True)
   last_report=time.monotonic()
  if time.monotonic()>deadline:
   process.terminate()
   raise RuntimeError('Mac verification timed out; see '+str(log_path))
  time.sleep(0.5)
 result_code=process.wait()
data=log_path.read_text(errors='replace')
for path in captures.glob('*.png'):
 shutil.copy2(path,output/path.name)
checks=[line.split('ISLAND_CHECK_PASS: ',1)[1] for line in data.splitlines() if 'ISLAND_CHECK_PASS: ' in line]
failures=[line for line in data.splitlines() if 'ISLAND_CHECK_FAIL:' in line]
screenshots=[p.name for p in output.glob('*.png')]
passed=result_code == 0 and 'ISLAND_VALIDATION_PASS:' in data and 'DIALOGUE_VALIDATION_COMPLETED:' in data and not failures and len(screenshots)>=15
if (root/'Saved/WeatherSystem/application_report.json').is_file():
 passed = passed and any('Fab Niagara rainfall 50%' in check for check in checks)
 passed = passed and any('Fab surface humidity matches weather' in check for check in checks)
report={'passed':passed,'process_exit':result_code,'checks':checks,'failures':failures,'screenshots':screenshots,'output':str(output)}
(output/'result.json').write_text(json.dumps(report,indent=2,ensure_ascii=False))
(root/'Saved/IslandMacValidation/latest.txt').write_text(str(output))
print('MAC_VALIDATION_RESULT:',json.dumps(report,ensure_ascii=False),flush=True)
if not passed:raise SystemExit(1)
