"""Inspect loaded swimming assets and capsule state on the connected Fold."""
from pathlib import Path
from datetime import datetime
import subprocess,shlex,time
root=Path(__file__).resolve().parents[2]
out=root/'Saved/Swimming/Inspection'/datetime.now().strftime('%Y%m%d-%H%M%S')
out.mkdir(parents=True)
adb=[str(Path.home()/'Library/Android/sdk/platform-tools/adb'),'-s','R5KL7064NZV']
def command(value):
    subprocess.run([*adb,'shell','am broadcast -a android.intent.action.RUN -p com.YourCompany.buildtest --es cmd '+shlex.quote(value)],capture_output=True,check=True)
with (out/'logcat.txt').open('w') as log:
    proc=subprocess.Popen([*adb,'logcat','-T','1','-v','threadtime','UE:V','UE4:V','*:S'],stdout=log,stderr=subprocess.STDOUT)
    try:
        for c in ['getall CapsuleComponent RelativeLocation','getall JumoLocomotionComponent SwimIdle',
                  'getall JumoLocomotionComponent SwimForward','getall FairySwimmingComponent EnterDepth',
                  'getall WaterBodyOceanComponent OceanExtents','getall CharacterMovementComponent MovementMode',
                  'getall CharacterMovementComponent Velocity']:
            command(c)
        command('BugItGo 3000 -14500 -35 0 -90 0')
        command('Walk')
        time.sleep(3)
        command('getall CapsuleComponent RelativeLocation')
        command('getall CharacterMovementComponent MovementMode')
        time.sleep(1)
        data=subprocess.run([*adb,'exec-out','screencap','-p'],capture_output=True,check=True).stdout
        (out/'deep_water.png').write_bytes(data[data.find(b'\x89PNG'):])
    finally:
        proc.terminate();proc.wait(timeout=10)
print(out)
for line in (out/'logcat.txt').read_text(errors='replace').splitlines():
    if any(s in line for s in ['RelativeLocation =','SwimIdle =','SwimForward =','EnterDepth =','OceanExtents =','MovementMode =','Velocity =','FAIRY_SWIM','JUMO_LOCOMOTION_STATE']):print(line)
