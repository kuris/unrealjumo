"""Install the verified full-cook APK, launch it, and record both island patrols."""
import argparse
import atexit
from datetime import datetime
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'Tools/WeatherTest'))
from verify_weather_package import verified_apk

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--device', default='R5KL7064NZV')
parser.add_argument('--seconds', type=int, default=240)
args = parser.parse_args()
apk = verified_apk(root)
adb = str(Path.home() / 'Library/Android/sdk/platform-tools/adb')
base = [adb, '-s', args.device]
folder = root / 'Saved/JumoAnimations/Device' / datetime.now().strftime('%Y%m%d-%H%M%S')
folder.mkdir(parents=True)
package = 'com.YourCompany.buildtest'


def run(name, *arguments, check=True):
    result = subprocess.run([*base, *arguments], capture_output=True, text=True)
    (folder / (name + '.txt')).write_text(result.stdout + result.stderr)
    if check:
        result.check_returncode()
    return result.stdout


def console(command):
    # ADB shell is a separate quoting boundary even with subprocess lists.
    return run('console_' + str(time.monotonic_ns()), 'shell', 'am broadcast -a android.intent.action.RUN -p ' + package + ' --es cmd ' + shlex.quote(command))


def screenshot(name):
    result = subprocess.run([*base, 'exec-out', 'screencap', '-p'], capture_output=True, check=True)
    offset = result.stdout.find(b'\x89PNG\r\n\x1a\n')
    if offset < 0:
        raise RuntimeError('No screenshot PNG returned')
    (folder / (name + '.png')).write_bytes(result.stdout[offset:])


# Keep the app active while observing a full patrol, then restore the prior setting.
previous_stay_on = run('stay_on_before', 'shell', 'settings', 'get', 'global', 'stay_on_while_plugged_in').strip()
restore_marker = root / 'Saved/JumoAnimations/original_stay_on.txt'
if restore_marker.is_file():
    previous_stay_on = restore_marker.read_text().strip()
else:
    restore_marker.write_text(previous_stay_on)
def restore_power():
    subprocess.run([*base, 'shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', previous_stay_on], capture_output=True)
    restore_marker.unlink(missing_ok=True)
atexit.register(restore_power)
run('stay_awake', 'shell', 'svc', 'power', 'stayon', 'usb')
model = run('model' , 'shell', 'getprop', 'ro.product.model').strip()
print('Installing on', model, args.device, flush=True)
installed = run('install', 'install', '-r', str(apk))
if 'Success' not in installed:
    raise RuntimeError('ADB did not confirm installation')
run('package', 'shell', 'dumpsys', 'package', package)
activity = run('activity', 'shell', 'cmd', 'package', 'resolve-activity', '--brief', package)
component = next((line.strip() for line in reversed(activity.splitlines()) if '/' in line), None)
if not component:
    raise RuntimeError('Launch activity not found')
run('stop', 'shell', 'am', 'force-stop', package)
run('wake', 'shell', 'input', 'keyevent', 'KEYCODE_WAKEUP')
run('dismiss_keyguard', 'shell', 'wm', 'dismiss-keyguard', check=False)
run('collapse_notifications', 'shell', 'cmd', 'statusbar', 'collapse', check=False)
log_path = folder / 'logcat.txt'
progress = {}
starts = set()
view_set = False
full_loops = False
pattern = re.compile(r'ISLAND_PATROL_PROGRESS: (\w+) reached=(\d+) target=(\d+) speed=([\d.]+) moved=([\d.]+) location=(.*)')
with log_path.open('w') as log:
    process = subprocess.Popen([*base, 'logcat', '-T', '1', '-v', 'threadtime', 'UE:V', 'UE4:V', 'Unreal:V', 'AndroidRuntime:E', '*:S'], stdout=log, stderr=subprocess.STDOUT)
    try:
        print(run('launch', 'shell', 'am', 'start', '-W', '-n', component), flush=True)
        deadline = time.monotonic() + args.seconds
        last_report = time.monotonic()
        while time.monotonic() < deadline:
            data = log_path.read_text(errors='replace')
            starts = set(re.findall(r'ISLAND_PATROL_START: (\w+)', data))
            for match in pattern.finditer(data):
                name, reached, target, speed, moved, location = match.groups()
                progress[name] = {'reached': int(reached), 'target': int(target), 'speed_cm_s': float(speed), 'moved_cm': float(moved), 'location': location}
            if time.monotonic() - last_report >= 25:
                print('PATROL_DEVICE_PROGRESS:', json.dumps(progress), flush=True)
                last_report = time.monotonic()
                screenshot('patrol_' + str(int(time.monotonic() - (deadline - args.seconds))))
            if {'seonbi', 'pojol'} <= progress.keys() and all(progress[name]['reached'] >= 96 for name in ('seonbi', 'pojol')):
                full_loops = True
                break
            if 'ISLAND_PATROL_INVALID:' in data or 'Fatal error:' in data or 'FATAL EXCEPTION' in data:
                raise RuntimeError('Runtime failure; see ' + str(log_path))
            time.sleep(2)
        screenshot('screen_final')
        run('resumed_activity', 'shell', 'dumpsys', 'activity', 'activities')
        run('pid', 'shell', 'pidof', package)
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
result = {'device': args.device, 'model': model, 'apk': str(apk), 'started': sorted(starts), 'progress': progress, 'both_completed_loop': full_loops, 'logs': str(folder)}
(folder / 'result.json').write_text(json.dumps(result, indent=2))
(root / 'Saved/JumoAnimations/latest_device.txt').write_text(str(folder))
print('PATROL_DEVICE_RESULT:', json.dumps(result), flush=True)
if not full_loops:
    raise SystemExit('Full patrol loop not verified; inspect progress and logs')
print('ISLAND_PATROL_DEVICE_PASS: both characters completed a loop; app left running', flush=True)
