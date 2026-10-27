"""Capture a patrol beside the player, with an unobstructed viewing angle."""
from pathlib import Path
import re,shlex,subprocess,time
root=Path(__file__).resolve().parents[2]
adb=[str(Path.home()/'Library/Android/sdk/platform-tools/adb'),'-s','R5KL7064NZV']
def command(text):
 return subprocess.run([*adb,'shell','am broadcast -a android.intent.action.RUN -p com.YourCompany.buildtest --es cmd '+shlex.quote(text)],capture_output=True,check=True)
def capture(name):
 b=subprocess.run([*adb,'exec-out','screencap','-p'],capture_output=True,check=True).stdout
 i=b.find(b'\x89PNG\r\n\x1a\n')
 if i<0:raise RuntimeError('Screenshot failed')
 (root/'Saved/JumoAnimations'/name).write_bytes(b[i:])
try:
 command('slomo 0.001')
 command('getall CapsuleComponent RelativeLocation')
 time.sleep(0.5)
 log=subprocess.run([*adb,'logcat','-d','-v','threadtime','-s','UE'],capture_output=True,check=True).stdout.decode(errors='replace')
 (root/'Saved/JumoAnimations/preview_logcat.txt').write_text(log)
 for index,name in [(0,'seonbi'),(1,'pojol')]:
  rows=re.findall(r'IslandPatrolCharacter_'+str(index)+r'\.CollisionCylinder\.RelativeLocation = \(X=([-\d.]+),Y=([-\d.]+),Z=([-\d.]+)\)',log)
  if not rows:raise RuntimeError('Could not locate '+name)
  x,y,z=map(float,rows[-1])
  command('BugItGo %.1f %.1f 190 -5 -5 0'%(x-550,y+300))
  command('Walk')
  command('slomo 1')
  time.sleep(0.35)
  command('slomo 0.001')
  capture(name+'_preview.png')
  print(name,rows[-1],flush=True)
 command('slomo 1')
finally:
 command('slomo 1')
 command('Walk')
