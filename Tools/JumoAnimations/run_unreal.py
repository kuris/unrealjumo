"""Run a project Unreal Python script and preserve its log."""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('script', type=Path)
parser.add_argument('--name')
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
output = root / 'Saved/JumoAnimations'
output.mkdir(parents=True, exist_ok=True)
name = args.name or args.script.stem
editor = '/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
command = [editor, str(root / 'buildtest.uproject'), '-run=pythonscript', '-script=' + str(args.script.resolve()), '-unattended', '-nop4', '-nosplash', '-nullrhi', '-stdout', '-abslog=' + str(output / (name + '.log'))]
with (output / (name + '_stdout.log')).open('w') as log:
    result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
print('Editor exit:', result.returncode, 'Log:', output / (name + '.log'))
raise SystemExit(result.returncode)
