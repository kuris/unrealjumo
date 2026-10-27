#!/usr/bin/env python3
"""Install the weather controls APK and record an interactive device test."""
import argparse
from datetime import datetime
from pathlib import Path
import subprocess
from verify_weather_package import verified_apk


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", help="ADB serial (optional when one physical device is connected)")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    adb = Path.home() / "Library/Android/sdk/platform-tools/adb"
    apk = verified_apk(root)
    devices = subprocess.run([str(adb), "devices"], check=True, capture_output=True, text=True).stdout
    connected = [line.split()[0] for line in devices.splitlines()[1:]
                 if len(line.split()) >= 2 and line.split()[1] == "device"
                 and not line.startswith("emulator-")]
    serial = args.device or (connected[0] if len(connected) == 1 else None)
    if not serial:
        parser.error("Connect and authorize one phone, or specify --device SERIAL.\n" + devices)
    command = [str(adb), "-s", serial]
    folder = root / "Saved/WeatherDeviceDiagnostics" / datetime.now().strftime("%Y%m%d-%H%M%S")
    folder.mkdir(parents=True)
    package = "com.YourCompany.buildtest"

    def capture(name, *arguments):
        result = subprocess.run([*command, *arguments], capture_output=True, text=True)
        (folder / name).write_text(result.stdout + result.stderr)
        return result

    result = capture("install.txt", "install", "-r", str(apk))
    print(result.stdout, flush=True)
    result.check_returncode()
    capture("package.txt", "shell", "dumpsys", "package", package)
    capture("display.txt", "shell", "wm", "size")
    capture("density.txt", "shell", "wm", "density")
    activity = capture("activity.txt", "shell", "cmd", "package", "resolve-activity", "--brief", package)
    component = next((line.strip() for line in reversed(activity.stdout.splitlines()) if "/" in line), None)
    if not component:
        raise RuntimeError("Could not resolve the app's launch activity; see activity.txt")
    capture("stop.txt", "shell", "am", "force-stop", package)
    with (folder / "logcat.txt").open("w") as log:
        process = subprocess.Popen([*command, "logcat", "-T", "1", "-v", "threadtime",
                                    "UE:V", "UE4:V", "Unreal:V", "AndroidRuntime:E", "*:S"],
                                   stdout=log, stderr=subprocess.STDOUT)
        try:
            result = capture("launch.txt", "shell", "am", "start", "-W", "-n", component)
            result.check_returncode()
            print("폰에서 날씨 버튼을 3번 누르고 슬라이더를 0%, 50%, 100%로 움직여주세요.", flush=True)
            print("이동하면서 달리기 ON/OFF, 점프 누르기/떼기도 확인해주세요.", flush=True)
            print("테스트를 마친 뒤 이 터미널에서 Enter를 누르면 로그와 화면을 저장합니다.", flush=True)
            input()
            capture("resumed_activity.txt", "shell", "dumpsys", "activity", "activities")
            capture("input.txt", "shell", "dumpsys", "input")
            screenshot = subprocess.run([*command, "exec-out", "screencap", "-p"],
                                        capture_output=True, check=True).stdout
            # Fold's screencap may prefix the PNG with a multiple-display warning.
            offset = screenshot.find(b"\x89PNG\r\n\x1a\n")
            if offset < 0:
                raise RuntimeError("Device did not return a PNG screenshot.")
            (folder / "screen.png").write_bytes(screenshot[offset:])
        finally:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    (root / "Saved/WeatherDeviceDiagnostics/latest.txt").write_text(str(folder))
    print("WEATHER_DEVICE_LOGS_SAVED:", folder, flush=True)


if __name__ == "__main__":
    main()
