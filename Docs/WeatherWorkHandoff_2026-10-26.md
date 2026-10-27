# 날씨 기능 인수인계 — 2026-10-26

## 최신 수정: 선비·포졸 임포트와 언덕 순찰

`Source/glb`의 선비·포졸을 메시·전용 스켈레톤·재질·텍스처·애니메이션 7개씩 가져왔습니다. 주모 애니메이션 7개도 기존 스켈레톤에 맞춰 `/Game/Characters/JumoAnimations`에 추가했습니다. 이전 임포트 실패는 GLB 경로 수정과 애니메이션 바인딩용 skeletal factory 활성화로 해결했습니다.

날씨 섬 맵 `/Game/Maps/LV_BeachIsland_Weather`에 `IslandPatrolCharacter` 두 개를 저장했습니다. 두 캐릭터는 언덕군 둘레의 서로 다른 경로를 반대 방향으로 걷습니다. 96개 지점과 캐릭터 폭 여유 구간의 지면·경사를 검사했습니다. CharacterMovement의 바닥·중력·충돌을 사용하고, 이동/정지에 따라 walk/idle을 재생합니다. 맵 백업은 `Saved/JumoAnimations/MapBackups`입니다.

Mac 플러그인 컴파일과 Android 전체 재쿠킹·패키징 성공, APK 지문 검증 통과, 보호 대상 8개 해시 일치. 폴드8 `SM-F971N / R5KL7064NZV`에 설치·실행했으며 두 캐릭터의 시작과 이동 로그를 확인했습니다. 한 바퀴 완주 검증을 진행 중입니다. 상세 도구와 에셋 경로는 [IslandCharacters.md](IslandCharacters.md)를 참고하세요.

이하 내용은 이전 단계 기록입니다.

## 최신 수정: 달리기·점프 버튼 및 비 영역 추적

화면 오른쪽 상단 아래에 달리기 ON/OFF와 점프 버튼을 추가했습니다. 달리기는 현재 플레이어 Character의 기존 MaxWalkSpeed를 저장하고 1.6배로 전환하며 OFF·위젯 제거 시 복구합니다. 점프 누르기/떼기는 기존 ACharacter::Jump/StopJumping을 호출합니다. 입력 로그는 CHARACTER_INPUT: run ON/OFF, jump pressed/released입니다. 조이스틱과 겹치지 않도록 버튼을 상단 기준 y=180/256에 배치했습니다.

기존 ThirdPerson BP_JumoCharacter에 IA_Jump 및 Touch Jump Start/End가 정의되어 있습니다. 현재 주모 ABP_Jumo의 에셋 참조에는 Idle/Walk만 확인됐으며 전용 달리기·점프 애니메이션은 추가하지 않았습니다. 이번 달리기는 기존 보행 애니메이션을 유지한 속도 전환입니다. 다른 스켈레톤의 Mannequin 조깅·점프 에셋을 주모에 바로 연결하지 않았습니다.

비는 플레이어 위치를 따라가도록 Rain 컴포넌트만 이동하고, 낙하 속도는 1600에서 700cm/s로 줄였습니다. 원본 맵·캐릭터·Config는 수정하지 않았습니다.

검증: UHT Android 헤더 생성 성공(3개 생성 파일), Widget/Actor/생성 모듈 Android arm64 컴파일 성공, 보호 대상 8개 SHA-256 일치. 전체 APK 빌드를 실행했으나 에이전트 sandbox에서 사용자 Library의 UnrealBuildTool Trace 백업 파일 접근이 거부되어 중단됐습니다. Saved/WeatherValidation/actions_build_stdout.log에 기록했습니다. **이번 변경을 포함한 APK는 아직 생성되지 않았습니다.** 사용자 Mac 터미널에서 build_android_weather.py 전체 빌드 후 diagnose_fold_weather.py로 실기기 버튼·동시 조이스틱 조작·점프 착지·비 추적을 확인해야 합니다.

## 15:27 실기기 로그 확인

Saved/WeatherDeviceDiagnostics/20261026-152540에서 APK 설치 Success, 날씨 버튼 입력 3회와 Rain→Clear→Rain 상태 전환, 강수량 슬라이더 0~100% 입력을 확인했습니다. 재질 생성 로그 WEATHER_RAIN_MATERIAL_PASS와 Android Vulkan/OpenGL 셰이더 컴파일 및 전체 패키징 성공을 확인했고 패키지 지문 검증도 통과했습니다. 캡처는 비 100% 상태이나 빗줄기가 뚜렷하게 보이지 않습니다. 런타임 재질 적용 성공을 직접 출력하는 로그는 없으므로 시각적 품질까지 통과한 것은 아닙니다.

현재 속도는 코드상 1600cm/s(16m/s), 12m 높이 구간을 약 0.75초에 통과합니다. 강수량 슬라이더는 속도를 조절하지 않습니다. 비는 Actor 기준 20×20m 고정 영역이고 캐릭터를 따라가지 않아 이동 후 영역 밖에서는 보이지 않을 수 있습니다. 캡처만으로 캐릭터가 해당 영역 밖인지 확정할 수 없습니다. 로그에 IoDispatcher 메모리 매핑 Error가 있으나 이후 맵과 UI 입력이 진행됐으므로 이를 크래시로 판단하지 않았습니다.

## 최신 수정: 투명한 빗물 재질

기본 큐브의 불투명 재질을 전용 /Game/Weather/M_ClearRain으로 교체하도록 수정했습니다. create_rain_material.py는 Unreal Python에서 반투명·Unlit·양면 재질(불투명도 0.14, 옅은 청백색)을 생성합니다. 조명 그림자 때문에 갈색·검정으로 보이는 현상을 피하고 모바일에서 굴절·SceneColor 비용을 추가하지 않습니다. 빗줄기 크기는 1.2×1.2×65cm에서 0.5×0.5×35cm로 줄였습니다. 실제 유체 굴절 효과는 사용하지 않습니다.

기존 날씨 맵을 읽을 때 BuildRain에서 재질을 적용하므로 맵을 재생성할 필요가 없습니다. Android 빌드 스크립트는 Mac 플러그인 빌드 후 Unreal Python으로 재질을 생성·저장하고 Content/Weather를 명시적으로 쿠킹합니다. 패키지 지문에 재질 생성 스크립트와 생성된 에셋도 포함합니다. 에셋 생성·패키징 및 기기 화면 검증은 아직 미완료입니다. 에이전트 환경에서는 UnrealEditor 실행이 종료 코드 134로 중단되어 사용자 Mac 터미널에서 전체 빌드가 필요합니다.

## 최신 상태: 폴드 입력 레이어 수정

14:54 전체 재쿠킹 APK의 기기 로그(`Saved/WeatherDeviceDiagnostics/20261026-145442/logcat.txt`)에서 비치 날씨 맵 로드와 맑음 시작을 확인했습니다. 수집된 로그에 이전 직렬화 크래시는 없었고 화면에 날씨 버튼·강수량 슬라이더가 표시됐으나, 버튼·슬라이더 입력 로그가 없었습니다. 사용자는 기본 이동 외 버튼이 반응하지 않는다고 보고했습니다.

엔진의 GameViewportSubsystem.cpp를 확인한 결과 AddToPlayerScreen의 플레이어 레이어 위에 모바일 기본 조이스틱의 전역 레이어가 놓입니다. 날씨 위젯을 AddToViewport(1000)으로 옮겨 조이스틱보다 위에서 터치를 받도록 수정했습니다. 비어 있는 UI 영역은 SelfHitTestInvisible을 유지합니다. 실행 시 `WEATHER_UI: controls added above mobile joystick overlay`, 버튼은 `WEATHER_INPUT: top weather button pressed`, 슬라이더는 `WEATHER_INPUT: rain slider ...` 로그를 남깁니다. 수정된 Actor의 Android arm64 컴파일은 통과했습니다. **이 입력 수정이 포함된 전체 APK 빌드와 실기기 입력 검증은 아직 미완료입니다.** 다른 게임 버튼의 입력도 함께 확인해야 합니다.

사용자 Mac 터미널에서 프로젝트 경로로 이동한 뒤 전체 빌드와 진단을 순서대로 실행합니다:

```bash
cd '/Users/mywellplus_release/Documents/Unreal Projects/buildtest'
python3 Tools/WeatherTest/build_android_weather.py --engine '/Users/Shared/Epic Games/UE_5.8'
python3 Tools/WeatherTest/diagnose_fold_weather.py
```

빌드 성공 후에만 진단 명령을 실행합니다. 진단은 최신 전체 쿠킹 APK를 설치·실행하고, 사용자가 버튼 3회와 슬라이더 0/50/100%를 조작한 뒤 Enter를 누르면 로그·화면을 저장합니다. Fold의 다중 디스플레이 경고가 PNG 앞에 붙는 경우도 처리했습니다.

## 현재 날씨 기능

BeginPlay에서 맑음으로 시작합니다. 맑음 태양 10,000 lux·스카이라이트 1.5·안개 0.002, 비 태양 6,500 lux·스카이라이트 1.2·안개 0.008입니다. 런타임 PostProcessComponent는 EV100 12·보정 +1.5를 적용합니다. 위젯은 상단 날씨 전환 버튼과 강수량 0~100% 슬라이더로 구성됩니다. 100%는 400개 빗줄기, 50%는 200개, 0%는 비 표시·이동 중지입니다. 맑음에서 양을 조절하면 다음 비 상태에 적용됩니다.

## 이전 시작 크래시와 배포 방식 수정

14:48 로그의 `Bad export index 589823/252`는 새 반영 속성의 네이티브 코드와 이전 쿠킹 데이터를 혼합한 테스트 APK에서 발생했습니다. 해당 네이티브 교체 APK는 `.apk.invalid`로 격리했습니다. 재사용하거나 설치하지 않습니다.

현재 build_android_weather.py는 Mac 쿠커 플러그인을 먼저 최신 소스로 빌드하고 이전 Android cook/stage 데이터를 Saved/WeatherCookBackups에 보존한 뒤 전체 재쿠킹합니다. 소스 지문과 APK SHA-256을 full_cook_manifest.json에 기록하며 설치·진단 도구는 일치하는 전체 쿠킹 APK만 허용합니다. 14:54 빌드 이후 앱이 비치 날씨 맵을 로드한 것을 확인했습니다. 현재 소스에는 추가 입력 수정이 있으므로 다시 빌드해야 합니다.

원본 보호 대상은 verify_protected.py로 확인합니다. 최신 커맨드릿 강수량 경계 검사와 -ValidateOnly 실행은 미검증입니다. Git 첫 커밋·push는 미완료이고 원격 주소도 미설정입니다.

## 이전 APK까지의 상태 (위 후속 수정이 최신)

| 항목 | 결과 |
| --- | --- |
| Mac arm64 플러그인 빌드 | 성공, 날씨 버튼·밝기 개선 코드 포함 |
| `LV_WeatherTest` | 생성·저장 및 과다 노출 보정 완료 |
| `LV_BeachIsland_Weather` | 원본 비치맵에서 복사·날씨 적용·저장 완료 |
| 맑음→비→맑음 상태 검사 | 통과, 빗줄기 인스턴스 400개 유지 |
| 비치 복사본에서 실제 비 표시 | 사용자가 확인. 밝기가 너무 어둡다는 피드백 후 개선 코드 반영 |
| Android 네이티브 빌드·쿠킹·패키징·아카이브 | 2026-10-26 14:21 성공, 전체 43.50초 |
| 개선 후 밝기·최상단 버튼의 실제 조작 | 확인 전 |
| 새 APK의 폴드 설치·실행·플러그인 로드 오류 해소 | 확인 전 |
| 원본 보호 대상 8개 파일 | SHA-256 일치 |
| Git | main 초기화 완료. 첫 커밋·push 미완료. 원격 주소 미설정 |

성공 로그: `WEATHER_BUILD_PASS`, `WEATHER_ANDROID_PACKAGE_PASS`, AutomationTool 종료 코드 0.

## APK와 설치

최신 APK(약 185MB):

```text
Saved/WeatherAndroidPackage/Android_ASTC/buildtest-arm64.apk
```

폴드를 USB로 연결한 뒤 프로젝트 루트의 **Mac 터미널**에서 실행합니다.

```bash
/bin/sh Saved/WeatherAndroidPackage/Android_ASTC/Install_buildtest-arm64.command
```

루트의 `Android_ASTC`에 남은 APK와 혼동하지 마세요. 새 출력은 `Saved/WeatherAndroidPackage`입니다. 패키지명은 `com.YourCompany.buildtest`입니다. 새 패키지는 실행 명령줄에 날씨 비치 복사본을 지정합니다. 기기에서 해당 맵으로 시작하는지는 설치 후 확인해야 합니다.

## 프로젝트와 보존 조건

- 프로젝트: `/Users/mywellplus_release/Documents/Unreal Projects/buildtest/buildtest.uproject`
- 엔진: `/Users/Shared/Epic Games/UE_5.8` (로그상 UE 5.8.2)
- Mac: Apple M4, arm64. Android: ASTC, arm64, Development.
- 원본 숲맵·비치맵·주모는 수정하지 않았습니다.
- 프로젝트 시작 레벨·게임모드·Config·`.uproject`는 변경하지 않았습니다.
- 새 비치 복사본의 지형·바다·캐릭터·게임모드는 원본에서 유지했습니다.

검사 대상은 `LV_ForestPath.umap`, `LV_BeachIsland.umap`, `BP_JumoCharacter.uasset`, 기존 Config 4개, `buildtest.uproject`입니다. 기준은 `Tools/WeatherTest/protected_baseline.json`입니다.

```bash
python3 Tools/WeatherTest/verify_protected.py
```

정상 출력: `PASS: 8 protected files unchanged`.

## 구현 내용

`Plugins/SimpleWeather`의 Runtime 모듈은 Blueprintable `ASimpleWeatherActor`를 제공합니다. 블루프린트에서 `SetWeather`, `ToggleWeather`, `GetWeather`를 호출할 수 있습니다. Editor 모듈은 별도 빈 테스트맵 생성용입니다.

- 맑음: 빗줄기 숨김, Tick 꺼짐, 태양 10,000 lux, 스카이라이트 1, 안개 0.002.
- 비: 빗줄기 표시·이동, 태양 4,000 lux, 스카이라이트 0.8, 안개 0.015.
- 밝기 개선 전 비 상태는 태양 1,000 lux, 스카이라이트 0.35, 안개 0.035였습니다.
- 빗줄기는 Engine Cube 메시 인스턴스 400개이며, 고정된 20m × 20m 영역에서 아래로 이동하고 순환합니다. 캐릭터 추적이나 섬 전체 강수는 구현하지 않았습니다.
- 외부 Niagara/마켓플레이스 에셋은 사용하지 않습니다.
- `Enable Test Key`를 켜면 R키 전환이 가능합니다.

### 비치 복사본

`Content/Maps/LV_BeachIsland_Weather.umap`은 원본 Actor 15개를 복사하고 날씨 Actor와 노출 볼륨을 추가한 맵입니다. PlayerStart 주변에 강수 영역을 배치했습니다. 복사본의 기존 태양·스카이라이트·대기·안개 컴포넌트만 숨겨 날씨 조명과 중복되지 않게 했습니다. 기존 태양 방향은 가져왔습니다.

프로젝트 자동 노출이 꺼져 있어 초기 조명이 하얗게 날아갔습니다. 프로젝트 설정 대신 복사본 내부 PostProcessVolume에서 EV100을 12로 고정했습니다. 기존 카메라 컴포넌트가 있으면 복사본에서 같은 노출을 적용합니다.

생성 기록: `Tools/WeatherTest/beach_weather_report.json`. 이 JSON의 화면 검사 상태는 생성 당시 `pending`이며, 이후 사용자 확인은 이 문서의 현재 상태가 최신입니다.

### 화면 최상단 날씨 버튼

`WeatherControlWidget.h/.cpp`에서 UMG 위젯을 코드로 생성합니다. 최상단 중앙, 안전 영역 안쪽 8 단위에 280 × 60 크기로 배치합니다. DPI 스케일·SafeZone을 사용하며 문구는 `날씨 전환: 맑음 → 비` / `날씨 전환: 비 → 맑음`입니다.

버튼 클릭·터치와 R키는 같은 날씨 상태를 변경하고 문구를 갱신합니다. 버튼 외 영역 입력은 통과시키도록 설정했습니다. Actor의 `Show Weather Button` 기본값은 true이며 로컬 플레이어에만 표시하고 EndPlay에서 제거합니다. 데스크톱에서는 GameAndUI와 마우스 커서를 사용합니다. Android에서는 이 데스크톱 입력 변경을 적용하지 않습니다. 폴드의 조이스틱·시점 조작·접힘/펼침·가로/세로 검증은 남아 있습니다.

### 프로젝트 기본 게임 모듈

Android에서 플러그인이 포함된 자체 실행 파일을 링크하도록 다음을 추가했습니다.

```text
Source/buildtest.Target.cs
Source/buildtestEditor.Target.cs
Source/buildtest/buildtest.Build.cs
Source/buildtest/buildtest.cpp
```

두 타깃이 `ExtraModuleNames.Add("buildtest")`를 사용합니다. 기본 게임 모듈은 `IMPLEMENT_PRIMARY_GAME_MODULE`로 게임 식별 심볼과 메모리 boilerplate를 제공합니다. 플러그인은 `.uplugin`의 `EnabledByDefault: true`로 활성화되며, 설치형 엔진과 충돌하던 타깃의 `EnablePlugins.Add`는 최종 코드에서 제거했습니다.

## 다시 빌드하는 명령

언리얼 변경 사항을 저장하고 에디터를 닫은 뒤 프로젝트 루트의 **Mac 터미널**에서 실행합니다.

Mac Editor용 플러그인만 업데이트:

```bash
python3 Tools/WeatherTest/build_and_generate.py \
  --engine '/Users/Shared/Epic Games/UE_5.8' --platform Mac --build-only
```

Android APK 재생성:

```bash
python3 Tools/WeatherTest/build_android_weather.py \
  --engine '/Users/Shared/Epic Games/UE_5.8'
```

Mac 스크립트의 `--build-only`는 기존 맵을 재생성하지 않습니다. Android 스크립트는 프로젝트 Game 타깃을 `-build`로 컴파일하며, 쿠킹에는 `-unrealexe`로 설치된 Mac 에디터 경로를 지정합니다. `-NoUBA -UBANoDetour -MaxParallelActions=2`를 사용합니다.

로그: `Tools/WeatherTest/build.log`, `android_build.log`, `Saved/WeatherAndroidLogs`. 에이전트 실행 환경에서는 사용자 Library 캐시·엔진 폴더·ADB에 권한 제한이 있어 전체 패키징은 사용자 터미널에서 성공했습니다.

## 도구 사용 위치

| 도구 | 실행 위치·용도 |
| --- | --- |
| `build_and_generate.py` | 시스템 터미널 python3, Mac 플러그인 빌드. 기존 맵이 있으면 `--build-only` |
| `build_android_weather.py` | 시스템 터미널 python3, Android 네이티브 빌드·패키징 |
| `verify_protected.py` | 시스템 터미널 python3, 원본 파일 해시 검사 |
| `fix_weather_exposure.py` | 언리얼 Cmd의 `py`, 빈 `LV_WeatherTest` 노출 보정. 이미 적용됨 |
| `create_beach_weather.py` | 언리얼 Cmd의 `py`, 비치맵 복사·날씨 적용. 이미 성공. 기존 복사본 덮어쓰기 거부 |

`import unreal` 도구는 시스템 Python에서 실행할 수 없습니다. `py`는 언리얼 내부 명령이며 zsh 명령이 아닙니다.

## 해결한 오류

| 오류 | 최종 조치 |
| --- | --- |
| UBA가 임시 PCH 파일을 찾지 못함 | 임시 경로 정규화, 직접 Editor 빌드, UBA 파일 가로채기 끄기 |
| 하얗게 날아가는 화면 | 테스트맵 내부 EV100 12 고정 |
| Android에서 SimpleWeather 로드 실패 | stock UnrealGame의 `-nocompile` 패키징 대신 프로젝트 Game 타깃 네이티브 빌드 |
| Editor target not found | 프로젝트 Editor 타깃 추가 |
| EnablePlugins가 Unique 환경 필요 | 명시적 타깃 활성화 제거, 플러그인 기본 활성화 사용 |
| GIsGameAgnosticExe·GInternalProjectName·FMemory 함수 미정의 | 기본 게임 모듈 추가. Android 단독 컴파일·심볼 검사·직접 링크 후 정식 빌드 성공 |
| buildtestEditor.target 없음 | 쿠킹에 설치된 에디터 절대 경로를 `-unrealexe`로 지정 |

새 APK에서 플러그인 로드 문제가 실제로 해소됐는지는 기기 검증 전입니다.

## Git과 남은 작업

Git은 `main` 초기화만 완료됐고 첫 커밋·원격 업로드는 아직 없습니다. 에이전트의 `git add`는 `.git/index.lock` 쓰기 제한으로 실패했습니다. 현재 원격 주소도 설정되지 않았습니다. 사용자에게 업로드할 GitHub 저장소 주소를 확인 중입니다.

추적 대상으로 준비한 것은 플러그인 소스·프로젝트 기본 게임 모듈·도구·문서·새 테스트맵 2개입니다. 원본 전체 에셋과 APK·빌드 캐시는 포함하지 않습니다. 이 자료를 다른 환경에서 사용하려면 원래 buildtest 프로젝트의 콘텐츠가 필요합니다. `.gitignore`는 Binaries·Intermediate·Saved·DerivedDataCache·Android_ASTC와 빌드 로그를 제외합니다.

1. 새 APK를 폴드에 설치하고 시작 맵·플러그인 로드를 확인합니다.
2. 최상단 날씨 버튼, R키, 개선된 밝기와 캐릭터 조작을 확인합니다.
3. 폴드의 터치·화면 회전·접힘/펼침을 확인합니다.
4. 원격 저장소를 정하고 첫 커밋·push를 완료합니다.

이후 상태가 바뀌면 이 문서를 갱신하고 미검증 항목을 완료로 미리 표시하지 않습니다.
