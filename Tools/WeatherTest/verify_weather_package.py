"""Verify that the APK came from a full cook with the current source files."""
import hashlib
import json
from pathlib import Path


def file_hash(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def source_fingerprint(root):
    files = [root / "Plugins/SimpleWeather/SimpleWeather.uplugin"]
    files.extend(p for p in (root / "Plugins/SimpleWeather/Data").rglob("*") if p.is_file())
    files.append(root / "Tools/WeatherTest/create_rain_material.py")
    rain_material = root / "Content/Weather/M_ClearRain.uasset"
    if rain_material.is_file():
        files.append(rain_material)
    for folder in (root / "Source", root / "Plugins/SimpleWeather/Source"):
        files.extend(p for p in folder.rglob("*") if p.suffix in (".cpp", ".h", ".cs"))
    # Patrol routes and their referenced character assets must match the cook.
    files.append(root / "Content/Maps/LV_BeachIsland_Weather.umap")
    for folder in ("Content/Characters/seonbi_final_rigged", "Content/Characters/pojol_final_rigged", "Content/Characters/JumoAnimations", "Content/Weather/Audio", "Content/WeatherSystem"):
        files.extend((root / folder).rglob("*.uasset"))
    return {str(p.relative_to(root)): file_hash(p) for p in sorted(files)}


def verified_apk(root):
    manifest = root / "Saved/WeatherAndroidPackage/full_cook_manifest.json"
    if not manifest.is_file():
        raise RuntimeError("Run build_android_weather.py first: no current full-cook package is available.")
    data = json.loads(manifest.read_text())
    if data["source_fingerprint"] != source_fingerprint(root):
        raise RuntimeError("Source changed after cooking. Rebuild the full Android package before installation.")
    apk = root / data["apk"]
    if not apk.is_file() or file_hash(apk) != data["apk_sha256"]:
        raise RuntimeError("APK is missing or differs from the fully cooked build.")
    return apk


if __name__ == "__main__":
    print(verified_apk(Path(__file__).resolve().parents[2]))
