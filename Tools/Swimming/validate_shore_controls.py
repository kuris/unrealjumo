"""Verify real jump/run touches after leaving a live swimming session for shore.

This is a separate restoration check, not a substitute for the natural shore
crossing test in validate_fold_swimming.py.
"""
from datetime import datetime
import json,re,shlex,struct,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[2]
out=root/'Saved/Swimming/ShoreControls'/datetime.now().strftime('%Y%m%d-%H%M%S')
out.mkdir(parents=True)
base=[str(Path.home()/'Library/Android/sdk/platform-tools/adb'),'-s','R5KL7064NZV']
def adb(*args):return subprocess.run([*base,*args],capture_output=True,check=True).stdout
def console(command):return adb('shell','am broadcast -a android.intent.action.RUN -p com.YourCompany.buildtest --es cmd '+shlex.quote(command))
def logs():return (out/'logcat.txt').read_text(errors='replace')
def wait(pattern,since=0):
    deadline=time.monotonic()+8
    while time.monotonic()<deadline:
        match=re.search(pattern,logs()[since:])
        if match:return match
        time.sleep(.1)
    raise RuntimeError('Missing input result: '+pattern)
report={'passed':False,'method':'Live swimming to shallow shore via development teleport, then real button touches'}
with (out/'logcat.txt').open('w') as file:
    logger=subprocess.Popen([*base,'logcat','-T','1','-v','threadtime','UE:V','UE4:V','*:S'],stdout=file,stderr=subprocess.STDOUT)
    try:
        adb('shell','am','start','-W','-n','com.YourCompany.buildtest/com.epicgames.unreal.SplashActivity')
        console('BugItGo 3000 -14500 -35 0 90 0');console('Walk')
        time.sleep(1)
        since=len(logs())
        console('BugItGo 3000 -13000 90 0 90 0');console('Walk')
        wait('FAIRY_SWIM_EXIT:',since)
        time.sleep(2)
        state=adb('shell','dumpsys','activity','activities').decode()
        if not any('com.YourCompany.buildtest' in s for s in state.splitlines() if 'ResumedActivity' in s):raise RuntimeError('Game is not in foreground')
        png=adb('exec-out','screencap','-p');png=png[png.find(b'\x89PNG'):]
        (out/'shore_controls.png').write_bytes(png)
        width,height=struct.unpack('>II',png[16:24])
        history=adb('logcat','-d','-s','UE').decode(errors='replace')
        row=re.findall(r'DIALOGUE_UI_RECTS: (.*)',history)[-1]
        vw,vh=map(int,re.search(r'viewport=(\d+),(\d+)',row).groups())
        def tap(name):
            x,y,w,h=map(int,re.search(r'\b'+name+r'=(-?\d+),(-?\d+),(\d+),(\d+)',row).groups())
            px,py=str(round((x+w/2)*width/vw)),str(round((y+h/2)*height/vh))
            adb('shell','input','swipe',px,py,px,py,'180')
        since=len(logs());tap('jump')
        wait('CHARACTER_INPUT: jump pressed',since)
        wait('JUMO_LOCOMOTION_STATE: Jump',since)
        report['jump_touch_after_exit']=True
        time.sleep(2)
        since=len(logs());tap('run')
        first=wait(r'CHARACTER_INPUT: run (ON|OFF),',since).group(1)
        since=len(logs());tap('run')
        wait('CHARACTER_INPUT: run '+('OFF' if first=='ON' else 'ON')+',',since)
        report['run_touch_after_exit']=True
        report['passed']=True
    except Exception as error:report['error']=str(error)
    finally:logger.terminate();logger.wait(timeout=10)
(out/'result.json').write_text(json.dumps(report,indent=2))
(root/'Saved/Swimming/latest_shore_controls.txt').write_text(str(out))
print(out,json.dumps(report),flush=True)
if not report['passed']:raise SystemExit(1)
