# 날씨 전환 테스트

## 비치맵 복사본에 적용

언리얼에서 Play를 중지하고 편집 중인 레벨과 에셋을 저장한 뒤, **출력 로그 하단 Cmd 입력란**에서 실행합니다. macOS 터미널의 Python에서는 실행할 수 없습니다.

```text
py "/Users/mywellplus_release/Documents/Unreal Projects/buildtest/Tools/WeatherTest/create_beach_weather.py"
```

도구는 원본 `/Game/Maps/LV_BeachIsland`를 언리얼의 템플릿 로더로 복사해 `/Game/Maps/LV_BeachIsland_Weather`로 저장합니다. 원본 지형·바다·캐릭터·게임모드는 복사본에 유지하며, 복사본의 기존 태양·하늘 조명·대기·안개 컴포넌트만 숨겨 날씨 Actor와 중복되지 않게 합니다. 태양 방향은 기존 방향을 가져오며, 노출은 복사본에서 EV100 12로 고정합니다. 원본 프로젝트 설정은 수정하지 않습니다.

기존 자동 활성화 카메라가 있으면 그 위치, 없으면 PlayerStart 주변에 20m × 20m 강수 영역을 배치합니다. 비는 해당 고정 영역에서만 보입니다. 성공 후 복사본이 열린 상태에서 Play, 뷰포트 클릭, R키로 테스트하세요. 밝기와 실제 화면에서의 전환은 시각적 검증이 필요합니다.

`BEACH_WEATHER_PASS`가 성공 표시입니다. 실행 시 맑음→비→맑음과 빗줄기 400개를 검사하고 원본 비치맵·숲맵·주모·Config·uproject의 해시를 확인합니다. 결과는 `Tools/WeatherTest/beach_weather_report.json`에 기록합니다. 같은 이름의 복사본이 이미 있거나 미저장 변경이 있으면 중단합니다. 공유 서브레벨이 발견되면 원본 보호를 위해 Actor 변경 전에 중단합니다. 중간 실패로 복사본이 남으면 덮어쓰지 않으므로 로그를 확인해야 합니다.

현재 상태: 적용 도구 문법과 설치된 UE API를 확인했고 보호 파일 8개가 일치합니다. 이 환경에서 에디터 실행이 실패하여 비치 복사본 생성과 화면 검증은 아직 수행하지 못했습니다.

## 구현한 코드

`ASimpleWeatherActor`는 재사용 가능한 Blueprintable C++ Actor입니다. `SetWeather(Clear/Rain)`, `ToggleWeather()`, `GetWeather()`를 블루프린트에서 호출할 수 있습니다. 상태는 즉시 전환됩니다.

- 맑음: 빗줄기를 숨기고 강한 햇빛, 밝은 간접광, 옅은 안개를 사용합니다. 애니메이션 Tick을 끕니다.
- 비: 400개의 가느다란 메시 인스턴스를 아래로 이동시키고, 햇빛과 간접광을 낮추며 안개를 짙게 합니다. 인스턴스는 재사용하며 바닥에 도달하면 위로 순환합니다.
- 외부 Niagara/마켓플레이스 에셋 없이 `/Engine/BasicShapes/Cube`를 빗줄기와 테스트 바닥에 사용합니다.
- Actor가 자신의 조명, 하늘, 안개만 제어합니다. 사용하려는 레벨에 직접 배치하세요. 이 Actor는 자체 조명 세트를 가지므로 기존 조명이 많은 레벨에는 중복 배치를 피하세요.
- `bEnableTestKey` 기본값은 꺼짐입니다. 생성된 테스트 레벨에서만 켜며, PIE 중 `R`로 전환합니다. 키 이벤트를 소비하지 않습니다.

이번 범위에는 구름 시뮬레이션, 강수 사운드, 젖은 표면, 날씨 보간, 네트워크 복제는 포함하지 않습니다. 비는 Actor 주변의 고정된 20m × 20m 테스트 영역에 표시됩니다.

## 실제 에디터에서 해야 할 작업

업데이트: 수정한 빌드 스크립트로 Mac arm64 컴파일이 성공했고, `generate.log`에서 `WEATHER_TEST_PASS`와 오류 0개를 확인했습니다. `LV_WeatherTest.umap`도 생성되어 에디터에서 로드됐습니다. 아래의 초기 권한 제한 설명은 이전 시도의 기록입니다. 기존 맵이 있으므로 생성 명령을 다시 실행하지 말고, 하얀 화면은 다음 노출 보정을 적용하세요.

## 테스트 맵 노출 보정

프로젝트의 자동 노출이 꺼져 있어 10,000 lux 테스트 조명이 화면을 하얗게 만들었습니다. 프로젝트 설정을 유지하면서 테스트 맵에만 무한 범위 PostProcessVolume을 추가하고 EV100을 12로 고정합니다. 비 전환 시에도 같은 노출을 유지하므로 조명이 어두워지는 차이를 확인할 수 있습니다. 이 값은 시각적 재검증이 필요합니다.

`LV_WeatherTest`를 열고 Play를 중지한 뒤, 출력 로그의 **Cmd** 입력란에서 실행하세요. 재컴파일 없이 현재 맵을 보정하고 저장합니다.

```text
py "Tools/WeatherTest/fix_weather_exposure.py"
```

상대 경로를 찾지 못하면 스크립트의 절대 경로를 사용하세요. 성공하면 `WEATHER_EXPOSURE_FIX_APPLIED`가 출력됩니다. 다른 맵이나 PIE 중에는 중단하며, 같은 이름의 보정 볼륨은 재사용합니다. 향후 새 맵 생성용 C++ 코드에도 같은 설정을 추가했으나 이 추가 수정은 재컴파일 전입니다.

## 초기 빌드 시도 기록

현재 `.umap` 또는 `.uasset`은 만들지 않았습니다. 설치된 UE 5.8의 빌드 도구가 샌드박스 밖의 `Library/Application Support/Epic/UnrealBuildTool/Trace*.uba` 파일에 쓰려다 권한 오류로 중단되었습니다. 재귀 빌드 모드에서도 UBA 초기화 오류로 중단되어 컴파일, 레벨 저장, PIE는 미검증 상태입니다. 아래 절차는 권한 제한이 없는 개발 환경에서 실행해야 합니다.

에디터를 닫고 프로젝트 루트에서 실행하세요. Python 표준 라이브러리만 사용합니다.

```sh
python3 Tools/WeatherTest/build_and_generate.py \
  --engine '/Users/Shared/Epic Games/UE_5.8' --platform Mac
```

Windows/Linux에서는 해당 UE 5.8 설치 경로와 `--platform Win64` 또는 `--platform Linux`를 지정하세요. 스크립트는 임시 호스트 프로젝트에서 `UnrealEditor` 대상의 플러그인 모듈을 직접 빌드하고, 그 결과의 Binaries/Intermediate만 플러그인 폴더에 복사합니다. 이후 실제 에디터의 `WeatherTest` 커맨드릿을 실행합니다. 원래 프로젝트를 C++ 프로젝트로 바꾸거나 `.uproject`를 수정하지 않습니다.

Mac 임시 경로의 `/var`와 `/private/var`를 정규화하고, `-NoUBA -UBANoDetour`로 UBA의 파일 입출력 가로채기를 끕니다. 이는 UBA가 PCH 파일을 찾지 못하는 오류에 대응한 설정입니다. UE 5.8에서는 UBA 스케줄러 이름이 로그에 남을 수 있습니다. Mac은 현재 시스템 아키텍처만 빌드하고, 메모리 사용을 줄이기 위해 동시 작업 수를 2로 제한합니다. 이 수정 이후 실제 컴파일 성공 여부는 아직 확인하지 않았습니다.

커맨드릿은 새 빈 월드에 날씨 Actor, 바닥, PlayerStart, 자동 활성화 카메라를 배치하고 `/Game/Maps/LV_WeatherTest`에 저장합니다. 게임모드 오버라이드는 지정하지 않습니다. 같은 이름의 레벨이 있으면 덮어쓰지 않고 실패합니다. 검증 실패 시 새 레벨을 저장하지 않습니다.

플러그인이 이미 빌드되어 있다면 다음 명령만 실행해도 됩니다.

```sh
'/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor' \
  "$PWD/buildtest.uproject" -run=WeatherTest -unattended -nop4 -nullrhi -stdout
```

커맨드릿을 쓰지 않는 경우: 에디터에서 새 **Empty Level**을 만들고 `Content/Maps/LV_WeatherTest`로 저장하세요. Place Actors에서 `Simple Weather Actor`를 배치하고 `Enable Test Key`를 켭니다. 바닥, PlayerStart, CameraActor를 배치하고 카메라의 Auto Activate for Player를 Player 0으로 지정합니다. 시작 레벨과 프로젝트 게임모드는 바꾸지 마세요. 블루프린트가 필요하면 이 C++ Actor를 부모로 삼아 새 Blueprint를 만드는 것은 선택 사항입니다.

## 테스트

1. 생성 명령의 종료 코드 0과 로그의 `WEATHER_TEST_PASS`를 확인합니다. 커맨드릿은 맑음의 가시성/Tick 꺼짐, 비의 가시성/Tick 켜짐, 빗줄기 위치 이동, 반복 전환 후 인스턴스 수 유지, 맑음 복귀 시 광량을 검사합니다. 현재 환경에서 실행된 결과는 아닙니다.
2. `LV_WeatherTest`를 직접 열고 Play를 누른 다음 뷰포트를 클릭합니다. 최초 상태에서 빗줄기가 없고 하늘과 바닥이 밝은지 확인합니다.
3. `R`을 눌러 빗줄기가 아래로 떨어지고 주변 조명이 어두워지는지 확인합니다. 다시 `R`을 눌러 맑음으로 복귀하는지 확인합니다. 여러 번 반복합니다.
4. PIE를 종료하고 다시 Play하여 맑음으로 시작하는지 확인합니다. 테스트 Actor의 Details에서 `Weather`를 바꾸거나 `Toggle Weather` 버튼으로도 확인할 수 있습니다.
5. `python3 Tools/WeatherTest/verify_protected.py`를 실행합니다. 기존 `LV_ForestPath`, `BP_JumoCharacter`, 모든 기존 Config 파일과 `.uproject`의 SHA-256을 작업 시작 시점과 비교합니다. 이 해시는 현재 작업 원본 기준이므로 다른 버전의 원본에서는 차이가 날 수 있습니다.

실제 수행한 검증: Python 도구 문법 및 도움말 확인, 보호 대상 파일 해시 비교, 설치된 UE 5.8 헤더와 사용하는 API 대조. 미수행: C++ 컴파일 성공 확인, `.umap` 생성, PIE 및 패키지 실행.

## 추가한 파일

- `.gitignore`, `README.md`, `Docs/WeatherTest.md`
- `Plugins/SimpleWeather/SimpleWeather.uplugin`
- `Plugins/SimpleWeather/Source/SimpleWeather/SimpleWeather.Build.cs`
- `Plugins/SimpleWeather/Source/SimpleWeather/Public/SimpleWeatherActor.h`
- `Plugins/SimpleWeather/Source/SimpleWeather/Private/SimpleWeatherActor.cpp`
- `Plugins/SimpleWeather/Source/SimpleWeather/Private/SimpleWeatherModule.cpp`
- `Plugins/SimpleWeather/Source/SimpleWeatherEditor/SimpleWeatherEditor.Build.cs`
- `Plugins/SimpleWeather/Source/SimpleWeatherEditor/Private/SimpleWeatherEditorModule.cpp`
- `Plugins/SimpleWeather/Source/SimpleWeatherEditor/Private/WeatherTestCommandlet.h`
- `Plugins/SimpleWeather/Source/SimpleWeatherEditor/Private/WeatherTestCommandlet.cpp`
- `Tools/WeatherTest/build_and_generate.py`
- `Tools/WeatherTest/verify_protected.py`
- `Tools/WeatherTest/protected_baseline.json`

에디터 실행에 성공한 뒤 추가되는 파일: `Content/Maps/LV_WeatherTest.umap`. 빌드 로그 및 생성된 Binaries/Intermediate는 커밋 대상에서 제외합니다. 기존 에셋을 수정·삭제하지 않았고 프로젝트 게임모드, 시작 레벨, 기존 설정도 변경하지 않았습니다.
