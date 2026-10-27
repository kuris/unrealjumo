"""Launch the verified APK and test surface swimming using real Android touches.

Development console only positions the fairy at the test beach and turns her
back toward shore. Ocean entry, swimming and shore exit use the mobile joystick.
"""
import argparse
from datetime import datetime
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import sys
import time

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'Tools/WeatherTest'))
from verify_weather_package import verified_apk
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--device',default='R5KL7064NZV')
args=parser.parse_args()
apk=verified_apk(root)
base=[str(Path.home()/'Library/Android/sdk/platform-tools/adb'),'-s',args.device]
package='com.YourCompany.buildtest'
out=root/'Saved/Swimming/Android'/datetime.now().strftime('%Y%m%d-%H%M%S')
out.mkdir(parents=True)
log_path=out/'logcat.txt'
checks=[]
shots=[]
display=None
logger=None
report={'passed':False,'device':args.device,'apk':str(apk),'output':str(out)}

def adb(*args,check=True):
    result=subprocess.run([*base,*args],capture_output=True,text=True,check=check)
    return result.stdout+result.stderr

def text():return log_path.read_text(errors='replace') if log_path.exists() else ''

def wait(predicate,name,seconds=25):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        data=text()
        if 'Fatal error:' in data or 'FATAL EXCEPTION' in data:raise RuntimeError('Android runtime crashed')
        result=predicate(data)
        if result:return result
        time.sleep(.2)
    raise RuntimeError('Timeout: '+name)

def check(condition,name):
    checks.append({'passed':bool(condition),'description':name})
    print(('PASS: ' if condition else 'FAIL: ')+name,flush=True)
    if not condition:raise RuntimeError(name)

def console(command):
    return adb('shell','am broadcast -a android.intent.action.RUN -p '+package+' --es cmd '+shlex.quote(command))

def ensure_foreground():
    state=adb('shell','dumpsys','activity','activities')
    resumed=[line for line in state.splitlines() if 'mResumedActivity' in line or 'topResumedActivity' in line]
    if not any(package in line for line in resumed):
        raise RuntimeError('Game moved to background; touch validation stopped')

def screenshot(name):
    global display
    ensure_foreground()
    data=subprocess.run([*base,'exec-out','screencap','-p'],capture_output=True,check=True).stdout
    start=data.find(b'\x89PNG\r\n\x1a\n')
    if start<0:raise RuntimeError('No screenshot data')
    data=data[start:]
    display=struct.unpack('>II',data[16:24])
    (out/(name+'.png')).write_bytes(data)
    shots.append({'file':name+'.png','size':list(display)})

def rect(name):
    data=text()
    rows=re.findall(r'DIALOGUE_UI_RECTS: (.*)',data)
    for row in reversed(rows):
        size=re.search(r'viewport=(\d+),(\d+)',row)
        value=re.search(r'\b'+re.escape(name)+r'=(-?\d+),(-?\d+),(\d+),(\d+)',row)
        if not size or not value:continue
        x,y,w,h=map(int,value.groups())
        if w<2 or h<2:continue
        sx,sy=display[0]/int(size[1]),display[1]/int(size[2])
        return x*sx,y*sy,w*sx,h*sy
    raise RuntimeError('No rendered UI rectangle: '+name)

def forward(name, duration=6000):
    ensure_foreground()
    x,y,w,h=rect('moveStick')
    cx,cy=round(x+w*.5),round(y+h*.5)
    top=round(cy-h*.34)
    check(0<=cx<display[0] and 0<=top<display[1],'Joystick touch coordinates inside unfolded screen')
    # A slow swipe can yield only tiny per-event axis deltas in the existing
    # touch widget. Drag promptly, then keep the same finger down as a player does.
    adb('shell','input','motionevent','DOWN',str(cx),str(cy))
    try:
        time.sleep(.5)
        adb('shell','input','motionevent','MOVE',str(cx),str(top))
        time.sleep(duration/1000.*.65)
        screenshot(name)
        time.sleep(duration/1000.*.35)
    finally:
        adb('shell','input','motionevent','UP',str(cx),str(top),check=False)

def tap(name):
    ensure_foreground()
    x,y,w,h=rect(name)
    px,py=str(round(x+w*.5)),str(round(y+h*.5))
    adb('shell','input','swipe',px,py,px,py,'180')

try:
    report['model']=adb('shell','getprop','ro.product.model').strip()
    adb('shell','input','keyevent','KEYCODE_WAKEUP')
    adb('shell','wm','dismiss-keyguard',check=False)
    adb('shell','am','force-stop',package)
    log_file=log_path.open('w')
    logger=subprocess.Popen([*base,'logcat','-T','1','-v','threadtime','UE:V','UE4:V','Unreal:V','AndroidRuntime:E','*:S'],stdout=log_file,stderr=subprocess.STDOUT)
    launched=adb('shell','am','start','-W','-n',package+'/com.epicgames.unreal.SplashActivity')
    (out/'launch.txt').write_text(launched)
    check('Status: ok' in launched,'Updated APK launches on Fold')
    wait(lambda d:'JUMO_LOCOMOTION_READY' in d and 'moveStick=' in d,'mobile controls loaded',60)
    time.sleep(2)
    screenshot('00_launch')
    check(max(display)/min(display)<1.6,'Device is unfolded')
    # Position in the manual-entry band, between 80cm and 100cm deep.
    console('BugItGo 3000 -13200 30 0 -90 0')
    console('Walk')
    time.sleep(1.5)
    screenshot('01_manual_button')
    wait(lambda d: bool(re.search(r'\bswim=\d+,\d+,\d+,\d+',d)),'swim button appears')
    mark=len(text())
    tap('swim')
    wait(lambda d:'FAIRY_SWIM_MANUAL: started=1' in d[mark:],'real swim-button touch starts swimming')
    check(True,'Conditional swim button starts swimming with real touch')
    time.sleep(1)
    screenshot('01_manual_swimming')
    console('BugItGo 3000 -13000 90 0 -90 0')
    console('Walk')
    time.sleep(2)
    screenshot('01_shallow_water')
    mark=len(text())
    forward('02_forward_swimming')
    wait(lambda d:'FAIRY_SWIM_ENTER:' in d[mark:],'real joystick enters water')
    check('SwimForward' in text()[mark:],'Real joystick plays forward swimming animation')
    wait(lambda d:'JUMO_LOCOMOTION_STATE: SwimIdle' in d[mark:],'released joystick treads water')
    time.sleep(2)
    screenshot('03_treading_water')
    check(True,'Released joystick treads water on device')
    mark=len(text())
    tap('run');tap('jump')
    time.sleep(.8)
    check('run touch received' not in text()[mark:] and 'jump touch received' not in text()[mark:], 'Run and jump buttons disabled during swimming')
    # Set a known offshore position facing the beach; the return itself is a
    # real touch-controlled swim across the depth boundary, not a teleport out.
    console('BugItGo 3000 -13500 -35 0 90 0')
    console('Walk')
    time.sleep(1)
    mark=len(text())
    forward('04_swim_toward_shore',duration=10000)
    wait(lambda d:'FAIRY_SWIM_EXIT:' in d[mark:],'touch-controlled shore exit')
    time.sleep(2)
    screenshot('05_shore_return')
    check(True,'Real joystick returns to shore and exits swimming')
    mark=len(text())
    tap('jump')
    wait(lambda d:'CHARACTER_INPUT: jump pressed' in d[mark:] and 'JUMO_LOCOMOTION_STATE: Jump' in d[mark:],'jump restored on shore')
    check(True,'Jump touch works after swimming')
    time.sleep(2)
    mark=len(text())
    tap('run')
    first=wait(lambda d: re.search(r'CHARACTER_INPUT: run (ON|OFF),',d[mark:]),'run button restored').group(1)
    mark=len(text())
    tap('run')
    expected='OFF' if first=='ON' else 'ON'
    wait(lambda d:'CHARACTER_INPUT: run '+expected+',' in d[mark:],'restore prior run selection')
    check(True,'Run touch restored after swimming')
    report['passed']=True
except Exception as error:
    report['error']=str(error)
    print('SWIMMING_DEVICE_ERROR:',error,flush=True)
    try:screenshot('failure')
    except Exception:pass
finally:
    if logger:
        logger.terminate()
        logger.wait(timeout=10)
        log_file.close()
    report.update({'checks':checks,'screenshots':shots})
    (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2))
    (root/'Saved/Swimming/latest_android.txt').write_text(str(out))
    print('SWIMMING_DEVICE_RESULT:',json.dumps(report,ensure_ascii=False),flush=True)
if not report['passed']:raise SystemExit(1)
