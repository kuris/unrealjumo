#!/usr/bin/env python3
"""Build the project's native Android target, then cook/package the weather map."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import subprocess
import sys
from verify_weather_package import file_hash, source_fingerprint


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    engine = args.engine.resolve() / "Engine"
    editor = engine / "Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
    if not editor.is_file():
        parser.error("The installed Mac UnrealEditor executable was not found.")
    if not (root / "Content/Maps/LV_BeachIsland_Weather.umap").is_file():
        parser.error("LV_BeachIsland_Weather must exist first.")
    verify = [sys.executable, str(root / "Tools/WeatherTest/verify_protected.py")]
    subprocess.run(verify, check=True)
    output = root / "Saved/WeatherAndroidPackage"
    output.mkdir(parents=True, exist_ok=True)
    manifest = output / "full_cook_manifest.json"
    manifest.unlink(missing_ok=True)
    # Reflected class changes require the cooker and game executable to agree.
    # Build the Mac cooker plugin from the same source before touching cook data.
    subprocess.run([
        sys.executable, str(root / "Tools/WeatherTest/build_and_generate.py"),
        "--engine", str(args.engine), "--platform", "Mac", "--build-only",
    ], check=True)
    # Generate the material before cooking; runtime builds cannot compile a new
    # material graph. Existing beach maps pick it up when their actor loads.
    subprocess.run([
        str(editor), str(root / "buildtest.uproject"), "-run=pythonscript",
        f"-script={root / 'Tools/WeatherTest/create_rain_material.py'}",
        "-unattended", "-nop4", "-nosplash", "-nullrhi", "-stdout",
        f"-abslog={root / 'Tools/WeatherTest/rain_material.log'}",
    ], check=True)
    if not (root / "Content/Weather/M_ClearRain.uasset").is_file():
        raise RuntimeError("Rain material generation did not produce M_ClearRain.uasset")
    fingerprint = source_fingerprint(root)
    # Preserve previous generated output, then force a fresh cook/stage.
    backup = root / "Saved/WeatherCookBackups" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    for relative in ("Saved/Cooked/Android_ASTC", "Saved/StagedBuilds/Android_ASTC"):
        path = root / relative
        if path.exists():
            destination = backup / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            path.rename(destination)
    # Keep AutomationTool's logs in the project rather than user Library.
    uat_logs = root / "Saved/WeatherAndroidLogs"
    uat_logs.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["uebp_LogFolder"] = str(uat_logs)
    environment["uebp_FinalLogFolder"] = str(uat_logs)
    log = root / "Tools/WeatherTest/android_build.log"
    command = [
        str(engine / "Build/BatchFiles/RunUAT.sh"), "BuildCookRun",
        f"-project={root / 'buildtest.uproject'}", "-target=buildtest",
        f"-unrealexe={editor}",
        "-platform=Android", "-cookflavor=ASTC", "-clientconfig=Development",
        "-build", "-cook", "-stage", "-package", "-archive",
        "-pak", "-iostore", "-compressed", "-nop4", "-utf8output",
        "-skipbuildeditor", "-nocompileeditor",
        "-ubtargs=-NoUBA -UBANoDetour -MaxParallelActions=2",
        "-map=/Game/Maps/LV_BeachIsland+/Game/Maps/LV_BeachIsland_Weather",
        f"-cookdir={root / 'Content/Weather'}",
        "-cmdline=/Game/Maps/LV_BeachIsland_Weather",
        f"-archivedirectory={output}",
    ]
    print("Building native Android game target with SimpleWeather enabled.", flush=True)
    try:
        with log.open("w") as stream:
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                       text=True, errors="replace", bufsize=1, env=environment)
            for line in process.stdout:
                print(line, end="", flush=True)
                stream.write(line)
            result = process.wait()
        if result:
            raise subprocess.CalledProcessError(result, command)
        package = output / "Android_ASTC/buildtest-arm64.apk"
        if not package.is_file():
            raise RuntimeError("Packaging returned success but no APK was found in " + str(output))
        if fingerprint != source_fingerprint(root):
            raise RuntimeError("Source changed during build; refusing to mark this APK ready.")
        manifest.write_text(json.dumps({
            "apk": str(package.relative_to(root)), "apk_sha256": file_hash(package),
            "source_fingerprint": fingerprint, "full_cook": True,
            "created_at": datetime.now().isoformat(),
        }, indent=2))
        print("WEATHER_ANDROID_PACKAGE_PASS:")
        print(package)
        print("Install this new package using its generated Install command; device launch is not yet verified.")
    finally:
        subprocess.run(verify, check=True)


if __name__ == "__main__":
    main()
