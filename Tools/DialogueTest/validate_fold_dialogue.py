#!/usr/bin/env python3
"""Install the full-cook APK and exercise real Android touches using rendered UI rectangles."""
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

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'Tools/WeatherTest'))
from verify_weather_package import verified_apk

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--device', default='R5KL7064NZV')
parser.add_argument('--skip-install', action='store_true')
parser.add_argument('--label', default='current-display', help='Describe the actual physical device state; no display simulation is applied.')
args = parser.parse_args()
apk = verified_apk(root)
adb = [str(Path.home() / 'Library/Android/sdk/platform-tools/adb'), '-s', args.device]
package = 'com.YourCompany.buildtest'
output = root / 'Saved/DialogueSystem/Android' / datetime.now().strftime('%Y%m%d-%H%M%S')
output.mkdir(parents=True)
log_path = output / 'logcat.txt'
checks = []
actions = []
shots = []
display_size = None
process = None
power_before = None
data_catalog = json.loads((root / 'Plugins/SimpleWeather/Data/Dialogues.json').read_text())
report = {'passed': False, 'device': args.device, 'label': args.label, 'apk': str(apk), 'output': str(output)}


def run(*arguments, check=True):
    result = subprocess.run([*adb, *arguments], capture_output=True, text=True)
    if check:
        result.check_returncode()
    return result.stdout + result.stderr


def log():
    return log_path.read_text(errors='replace') if log_path.exists() else ''


def wait_for(predicate, description, seconds=15):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        current = log()
        if 'Fatal error:' in current or 'FATAL EXCEPTION' in current or 'DIALOGUE_DATA_ERROR:' in current:
            raise RuntimeError('Game runtime failure; inspect ' + str(log_path))
        value = predicate(current)
        if value:
            return value
        time.sleep(0.15)
    raise RuntimeError('Timeout: ' + description)


def check(condition, description):
    checks.append({'passed': bool(condition), 'description': description})
    print(('PASS: ' if condition else 'FAIL: ') + description, flush=True)
    if not condition:
        raise RuntimeError(description)


def console(command):
    actions.append({'console': command})
    return run('shell', 'am broadcast -a android.intent.action.RUN -p ' + package + ' --es cmd ' + shlex.quote(command))


def screenshot(name):
    global display_size
    binary = subprocess.run([*adb, 'exec-out', 'screencap', '-p'], capture_output=True, check=True).stdout
    offset = binary.find(b'\x89PNG\r\n\x1a\n')
    if offset < 0:
        raise RuntimeError('Device did not return PNG data')
    binary = binary[offset:]
    (output / (name + '.png')).write_bytes(binary)
    width, height = struct.unpack('>II', binary[16:24])
    display_size = (width, height)
    shots.append({'file': name + '.png', 'width': width, 'height': height})
    return width, height


def rectangles(text):
    rows = re.findall(r'DIALOGUE_UI_RECTS: (.*)', text)
    if not rows:
        return None
    row = rows[-1]
    values = {name: tuple(map(int, values)) for name, *values in re.findall(r'(\w+)=(-?\d+),(-?\d+),(-?\d+),(-?\d+)', row)}
    dimensions = re.findall(r'viewport=(\d+),(\d+)', row)
    if not dimensions:
        dimensions = re.findall(r'Resizing viewport.*?, (\d+) x (\d+)', text)
    if display_size and dimensions:
        render_width, render_height = map(int, dimensions[-1])
        if render_width > 0 and render_height > 0:
            sx, sy = display_size[0] / render_width, display_size[1] / render_height
            values = {name: (round(x * sx), round(y * sy), round(w * sx), round(h * sy)) for name, (x, y, w, h) in values.items()}
            report['render_viewport'] = [render_width, render_height]
            report['actual_screen'] = list(display_size)
    values['raw'] = row
    return values


def ui_with(control, npc=None, node=None):
    def predicate(text):
        result = rectangles(text)
        if not result or control not in result or result[control][2] <= 1 or result[control][3] <= 1:
            return None
        if npc and 'npc=' + npc + ' ' not in result['raw']:
            return None
        if node and 'node=' + node + ' ' not in result['raw']:
            return None
        return result
    return wait_for(predicate, f'UI rectangle {control} / {npc} / {node}')


def tap(control, npc=None, node=None):
    rects = ui_with(control, npc, node)
    x, y, width, height = rects[control]
    actions.append({'tap': control, 'rect': [x, y, width, height], 'layout': rects['raw']})
    run('shell', 'input', 'tap', str(x + width // 2), str(y + height // 2))
    time.sleep(0.3)


def check_dialogue_layout(npc, node, screenshot_name):
    rects = ui_with('close', npc, node)
    width, height = screenshot(screenshot_name)
    check('paper' in rects and 'speech' in rects and 'speaker' in rects, npc + ' panel, message and name have actual layout rectangles')
    px, py, pw, ph = rects['paper']
    check(px >= 0 and py >= 0 and px + pw <= width and py + ph <= height, npc + ' entire message panel stays on screen')
    for control in ('close', 'speech', 'speaker', 'choice0', 'choice1', 'choice2'):
        if control not in rects:
            continue
        x, y, w, h = rects[control]
        check(x >= px - 1 and y >= py - 1 and x + w <= px + pw + 1 and y + h <= py + ph + 1,
              npc + ' ' + node + ' ' + control + ' not clipped by panel')
    if 'choice2' in rects:
        for first, second in (('choice0', 'choice1'), ('choice1', 'choice2')):
            a, b = rects[first], rects[second]
            check(a[1] + a[3] <= b[1] + 1, npc + ' choices do not overlap: ' + first + '/' + second)


def node_seen(npc, node_id, since):
    record = next(n for n in data_catalog['npcs'] if n['id'] == npc)
    node = next(n for n in record['nodes'] if n['id'] == node_id)
    wait_for(lambda text: f'DIALOGUE_NODE: npc={npc} node={node_id} text={node["text"]}' in text[since:], npc + ' ' + node_id)
    check(True, npc + ' real touch opens correct dialogue node: ' + node_id)


def approach(npc, actor_name):
    console('slomo 0.05')
    since = len(log())
    console('getall CapsuleComponent RelativeLocation')
    pattern = re.compile(re.escape(actor_name) + r'\.CollisionCylinder\.RelativeLocation = \(X=([-\d.]+),Y=([-\d.]+),Z=([-\d.]+)\)')
    match = wait_for(lambda text: (pattern.findall(text[since:]) or [None])[-1], 'NPC capsule position')
    x, y, z = map(float, match)
    console(f'BugItGo {x - 160:.2f} {y - 120:.2f} {z:.2f} 0 0 0')
    console('Walk')
    console('slomo 1')
    rects = ui_with('talk', npc)
    wait_for(lambda text: 'bubble' in (rectangles(text) or {}), npc + ' head bubble')
    time.sleep(0.4)
    width, height = screenshot(npc + '_approach')
    rects = ui_with('talk', npc)
    for control in ('talk', 'bubble'):
        x, y, w, h = rects[control]
        check(x >= 0 and y >= 0 and x + w <= width and y + h <= height, npc + ' ' + control + ' entirely inside actual screen')
    tx, ty, tw, th = rects['talk']
    for control in ('run', 'jump', 'weather', 'slider'):
        if control not in rects:
            continue
        x, y, w, h = rects[control]
        overlap = min(tx + tw, x + w) > max(tx, x) and min(ty + th, y + h) > max(ty, y)
        check(not overlap, npc + ' talk does not overlap ' + control)
    check('moveStick' in rects and 'lookStick' in rects, 'actual legacy movement/look stick bounds detected')
    if 'moveStick' in rects:
        jx, jy, jw, jh = rects['moveStick']
        for control in ('run', 'jump'):
            x, y, w, h = rects[control]
            check(x >= jx + jw - 2 and x + w <= width and y >= 0 and y + h <= height,
                  npc + ' ' + control + ' beside left movement stick without overlap')
        check('legacyJump' not in rects, 'duplicate right-side legacy jump button hidden')


def wait_closed(npc, since):
    wait_for(lambda text: f'DIALOGUE_ENDED: npc={npc} pawnInput=1 movement=1 moveIgnored=0 lookIgnored=0' in text[since:], 'input restored after ' + npc)
    expected_view = re.findall(r'DIALOGUE_STARTED: npc=' + npc + r' previousView=(\S+)', log())[-1]
    wait_for(lambda text: 'DIALOGUE_CAMERA_RESTORED: target=' + expected_view in text[since:], 'original camera restored')
    check(True, npc + ' restores movement/look/jump and camera after real close touch')
    ui_with('talk', npc)


try:
    report['model'] = run('shell', 'getprop', 'ro.product.model').strip()
    power_before = run('shell', 'settings', 'get', 'global', 'stay_on_while_plugged_in').strip()
    run('shell', 'svc', 'power', 'stayon', 'usb')
    run('shell', 'input', 'keyevent', 'KEYCODE_WAKEUP')
    run('shell', 'wm', 'dismiss-keyguard', check=False)
    if not args.skip_install:
        installation = run('install', '-r', str(apk))
        (output / 'install.txt').write_text(installation)
        check('Success' in installation, 'verified full-cook APK installed')
    run('shell', 'am', 'force-stop', package)
    stream = log_path.open('w')
    process = subprocess.Popen([*adb, 'logcat', '-T', '1', '-v', 'threadtime', 'UE:V', 'UE4:V', 'AndroidRuntime:E', '*:S'], stdout=stream, stderr=subprocess.STDOUT)
    launch = run('shell', 'am', 'start', '-W', '-n', package + '/com.epicgames.unreal.SplashActivity')
    (output / 'launch.txt').write_text(launch)
    check('Status: ok' in launch, 'Android game activity launched')
    wait_for(lambda text: 'DIALOGUE_READY:' in text and 'ISLAND_PATROL_START: pojol' in text, 'weather map and dialogue loaded', 50)
    run('shell', 'am', 'start', '-W', '-n', package + '/com.epicgames.unreal.GameActivity')
    since = len(log())
    console('getall IslandPatrolCharacter PatrolName')
    def find_names(text):
        found = dict((name, actor) for actor, name in re.findall(r'(IslandPatrolCharacter_\d+)\.PatrolName = \"?(seonbi|pojol)', text[since:]))
        return found if set(found) == {'seonbi', 'pojol'} else None
    names = wait_for(find_names, 'NPC actor names')
    check(set(names) == {'seonbi', 'pojol'}, 'both expected NPCs are present')
    screenshot('00_exploration')
    for npc in ('seonbi', 'pojol'):
        approach(npc, names[npc])
        since = len(log())
        tap('talk', npc)
        node_seen(npc, 'greeting', since)
        time.sleep(0.6)
        check_dialogue_layout(npc, 'greeting', npc + '_greeting')
        rects = ui_with('choice0', npc, 'greeting')
        check('run' not in rects and 'jump' not in rects and 'weather' not in rects, npc + ' other controls hidden in conversation')
        check('touchUiVisible=0' in rects['raw'], npc + ' original joystick overlay hidden during conversation')
        for index, branch in ((0, 'sound' if npc == 'seonbi' else 'stranger'), (1, 'tavern' if npc == 'seonbi' else 'sighting')):
            since = len(log())
            tap('choice' + str(index), npc, 'greeting')
            node_seen(npc, branch, since)
            check_dialogue_layout(npc, branch, npc + '_' + branch)
            if index == 0:
                since = len(log())
                tap('choice1', npc, branch)
                node_seen(npc, 'greeting', since)
        since = len(log())
        tap('close', npc)
        wait_closed(npc, since)
        screenshot(npc + '_closed')
        since = len(log())
        tap('talk', npc)
        node_seen(npc, 'greeting', since)
        since = len(log())
        tap('choice2', npc, 'greeting')
        wait_closed(npc, since)
        screenshot(npc + '_exit_choice')
    # Confirm post-dialogue buttons receive real Android touch input.
    rects = ui_with('talk', 'pojol')
    for control, pattern in (('weather', 'WEATHER_INPUT: top weather button pressed'), ('run', 'CHARACTER_INPUT: run ON'), ('run', 'CHARACTER_INPUT: run OFF'), ('jump', 'CHARACTER_INPUT: jump pressed')):
        since = len(log())
        tap(control)
        wait_for(lambda text, p=pattern: p in text[since:], 'post-dialogue touch ' + control)
        check(True, 'post-dialogue real touch: ' + control + ' / ' + pattern)
        if control == 'weather':
            time.sleep(1.5)
            screenshot('post_dialogue_rain')
        if control == 'jump':
            screenshot('post_dialogue_jump')
    time.sleep(1.8)
    screenshot('end_controls_restored')
    report['pid'] = run('shell', 'pidof', package).strip()
    report['passed'] = True
except Exception as error:
    report['error'] = str(error)
    print('DIALOGUE_ANDROID_ERROR:', error, flush=True)
    try:
        screenshot('failure')
    except Exception:
        pass
finally:
    try:
        console('slomo 1')
    except Exception:
        pass
    if process:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        stream.close()
    if power_before is not None:
        run('shell', 'settings', 'put', 'global', 'stay_on_while_plugged_in', power_before, check=False)
    report.update({'checks': checks, 'actions': actions, 'screenshots': shots})
    (output / 'result.json').write_text(json.dumps(report, indent=2, ensure_ascii=False))
    (root / 'Saved/DialogueSystem/Android/latest.txt').write_text(str(output))
    print('DIALOGUE_ANDROID_RESULT:', json.dumps(report, ensure_ascii=False), flush=True)
if not report['passed']:
    raise SystemExit(1)
