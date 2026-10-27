"""Run real rendered ocean entry/idle/movement/shore integration checks on Mac."""
from datetime import datetime
import json
from pathlib import Path
import subprocess
import time

root=Path(__file__).resolve().parents[2]
out=root/'Saved/Swimming/Validation'/datetime.now().strftime('%Y%m%d-%H%M%S')
out.mkdir(parents=True)
editor=Path('/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor')
command=[str(editor),str(root/'buildtest.uproject'),'/Game/Maps/LV_BeachIsland_Weather',
    '-game','-windowed','-ResX=1280','-ResY=800','-ForceRes','-nosplash','-unattended','-nop4',
    '-SwimmingValidate','-SwimmingValidationExit','-SwimmingValidationOutput='+str(out),
    '-stdout','-forcelogflush','-abslog='+str(out/'engine.log')]
with (out/'stdout.log').open('w') as log:
    process=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT)
    print('SWIMMING_VALIDATION_STARTED',process.pid,out,flush=True)
    deadline=time.monotonic()+240
    progress=time.monotonic()
    while process.poll() is None:
        if time.monotonic()>deadline:
            process.terminate()
            process.wait(timeout=20)
            raise RuntimeError('Swimming validation timeout: '+str(out))
        if time.monotonic()-progress>15:
            data=(out/'stdout.log').read_text(errors='replace')
            print('\n'.join([s for s in data.splitlines() if 'SWIM_CHECK_' in s or 'FAIRY_SWIM_' in s][-3:]),flush=True)
            progress=time.monotonic()
        time.sleep(0.5)
data=(out/'stdout.log').read_text(errors='replace')
checks=[s.split('SWIM_CHECK_PASS: ',1)[1] for s in data.splitlines() if 'SWIM_CHECK_PASS: ' in s]
failures=[s for s in data.splitlines() if 'SWIM_CHECK_FAIL:' in s]
screenshots=[s.name for s in out.glob('*.png')]
report={'passed':process.returncode==0 and 'SWIMMING_VALIDATION_PASS' in data and not failures and len(screenshots)==5,
    'exit_code':process.returncode,'checks':checks,'failures':failures,'screenshots':screenshots,'output':str(out)}
(out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2))
(root/'Saved/Swimming/latest_validation.txt').write_text(str(out))
print('SWIMMING_VALIDATION_RESULT',json.dumps(report,ensure_ascii=False),flush=True)
if not report['passed']:raise SystemExit(1)
