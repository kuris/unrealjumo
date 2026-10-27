#!/usr/bin/env python3
"""Build this plugin with UE, then ask the real editor to generate LV_WeatherTest."""
import argparse
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, type=Path, help="UE_5.8 directory (contains Engine)")
    parser.add_argument("--platform", choices=("Mac", "Win64", "Linux"), required=True)
    parser.add_argument("--build-only", action="store_true", help="Update plugin binaries without generating or modifying any maps")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    engine = args.engine.resolve() / "Engine"
    plugin = root / "Plugins/SimpleWeather"
    if not args.build_only and (root / "Content/Maps/LV_WeatherTest.umap").exists():
        parser.error("LV_WeatherTest already exists; this tool refuses to overwrite it.")
    batch = engine / "Build/BatchFiles"
    build = batch / {
        "Mac": "Mac/Build.sh",
        "Win64": "Build.bat",
        "Linux": "Linux/Build.sh",
    }[args.platform]
    binaries = engine / "Binaries" / args.platform
    editor = binaries / {
        "Mac": "UnrealEditor.app/Contents/MacOS/UnrealEditor",
        "Win64": "UnrealEditor-Cmd.exe",
        "Linux": "UnrealEditor-Cmd",
    }[args.platform]
    subprocess.run([sys.executable, str(root / "Tools/WeatherTest/verify_protected.py")], check=True)
    with tempfile.TemporaryDirectory(prefix="SimpleWeatherBuild-") as temporary:
        # Canonical paths avoid /var versus /private/var aliases in compiler outputs.
        host = Path(temporary).resolve() / "HostProject"
        host_plugin = host / "Plugins/SimpleWeather"
        shutil.copytree(plugin, host_plugin, ignore=shutil.ignore_patterns(
            "Binaries", "Intermediate", ".DS_Store"))
        project = host / "HostProject.uproject"
        project.write_text(json.dumps({
            "FileVersion": 3,
            "Plugins": [{"Name": "SimpleWeather", "Enabled": True}],
        }))
        # Build the editor modules directly: BuildPlugin does not forward -NoUBA
        # in UE 5.8 and also builds game/shipping targets unnecessary for this map.
        command = [
            str(build), "UnrealEditor", args.platform, "Development",
            f"-Project={project}", f"-plugin={host_plugin / 'SimpleWeather.uplugin'}",
            "-NoUBA", "-UBANoDetour", "-NoHotReload", "-NoUBTMakefiles", "-MaxParallelActions=2",
            f"-Log={root / 'Tools/WeatherTest/build.log'}",
        ]
        if args.platform == "Mac":
            command.append("-architecture=" + (
                "arm64" if platform.machine().lower() in ("arm64", "aarch64") else "x64"))
        subprocess.run(command, check=True)
        # Copy generated output only; never replace project assets or settings.
        for name in ("Binaries", "Intermediate"):
            if (host_plugin / name).exists():
                shutil.copytree(host_plugin / name, plugin / name, dirs_exist_ok=True)
    if args.build_only:
        subprocess.run([sys.executable, str(root / "Tools/WeatherTest/verify_protected.py")], check=True)
        print("WEATHER_BUILD_PASS: plugin updated; existing maps were not regenerated.")
        return
    subprocess.run([
        str(editor), str(root / "buildtest.uproject"), "-run=WeatherTest",
        "-unattended", "-nop4", "-nosplash", "-nullrhi", "-stdout",
        "-FullStdOutLogOutput", f"-abslog={root / 'Tools/WeatherTest/generate.log'}",
    ], check=True)
    subprocess.run([sys.executable, str(root / "Tools/WeatherTest/verify_protected.py")], check=True)


if __name__ == "__main__":
    main()
