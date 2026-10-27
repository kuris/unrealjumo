#!/bin/sh
set -eu
project_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
adb_path="${ANDROID_HOME:-$HOME/Library/Android/sdk}/platform-tools/adb"
apk_path=$(python3 "$project_root/Tools/WeatherTest/verify_weather_package.py")
if [ ! -x "$adb_path" ]; then
    echo "Android SDK adb was not found: $adb_path" >&2
    exit 1
fi
if [ ! -f "$apk_path" ]; then
    echo "Weather controls APK was not found: $apk_path" >&2
    exit 1
fi
"$adb_path" install -r "$apk_path"
