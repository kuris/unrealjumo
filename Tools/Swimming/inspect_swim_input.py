"""Diagnose real joystick acceleration while the fairy is swimming."""
from pathlib import Path
import argparse
from datetime import datetime
import subprocess,shlex,time
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--y',type=float,default=-15000)
parser.add_argument('--ready',action='store_true',help='Restart and leave the fairy at the swim-button beach, without touch injection.')
args=parser.parse_args()
out=root/'Saved/Swimming/InputInspection'/datetime.now().strftime('%Y%m%d-%H%M%S')
out.mkdir(parents=True)
base=[str(Path.home()/'Library/Android/sdk/platform-tools/adb'),'-s','R5KL7064NZV']
def adb(*args):return subprocess.run([*base,*args],capture_output=True,text=True,check=True).stdout
def console(value):return adb('shell','am broadcast -a android.intent.action.RUN -p com.YourCompany.buildtest --es cmd '+shlex.quote(value))
if args.ready:
    adb('shell','am','force-stop','com.YourCompany.buildtest')
    adb('shell','am','start','-W','-n','com.YourCompany.buildtest/com.epicgames.unreal.SplashActivity')
    time.sleep(4)
    console('BugItGo 3000 -13200 30 0 -90 0');console('Walk')
    time.sleep(2)
    image=subprocess.run([*base,'exec-out','screencap','-p'],capture_output=True,check=True).stdout
    (out/'ready_to_play.png').write_bytes(image[image.find(b'\x89PNG'):])
    print('READY_TO_PLAY',out,flush=True)
    raise SystemExit(0)
with (out/'logcat.txt').open('w') as log:
    logger=subprocess.Popen([*base,'logcat','-T','1','-v','threadtime','UE:V','UE4:V','*:S'],stdout=log,stderr=subprocess.STDOUT)
    try:
        adb('shell','am','start','-W','-n','com.YourCompany.buildtest/com.epicgames.unreal.SplashActivity')
        console(f'BugItGo 3000 {args.y} -35 0 90 0');console('Walk')
        time.sleep(2)
        adb('shell','input','motionevent','DOWN','316','1533')
        adb('shell','input','motionevent','MOVE','316','1300')
        for _ in range(3):
            time.sleep(1)
            for command in ['getall CharacterMovementComponent Acceleration','getall CharacterMovementComponent Velocity',
                'getall CapsuleComponent RelativeLocation','getall OceanBoxCollisionComponent BodyInstance']:
                console(command)
        time.sleep(.3)
    finally:
        adb('shell','input','motionevent','UP','316','1300')
        logger.terminate();logger.wait(timeout=10)
print(out)
for line in (out/'logcat.txt').read_text(errors='replace').splitlines():
    if ('BP_JumoCharacter' in line and any(s in line for s in ['Acceleration =','Velocity =','RelativeLocation ='])) or 'CollisionProfileName=' in line:
        print(line[:1200])
