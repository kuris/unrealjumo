# 2026-10-26 구현 보고서

작성일: 2026-10-27. 이 문서는 10월 26일에 구현한 기능과 당시 남아 있는 검증 자료를 정리합니다. 작업 대상은 UE 5.8.2의 `buildtest` 프로젝트와 날씨 섬 `/Game/Maps/LV_BeachIsland_Weather`입니다.

## 구현 요약

| 기능 | 구현한 동작 | 주요 코드·도구 |
| --- | --- | --- |
| 날씨 전환 | 맑음 시작, 화면 상단 버튼·R키로 맑음/비 전환 | `SimpleWeatherActor`, `WeatherControlWidget` |
| 강수량 | 슬라이더 0~100%, 비 400개 기준 표시 개수 변경 | `SimpleWeatherActor`, `create_rain_material.py` |
| 모바일 입력 | 조이스틱 위에서 날씨·강수량·달리기·점프 입력 처리 | `WeatherControlWidget` |
| 주모 동작 | Idle/Walk/Run/Jump 전환, 달리기 속도 증가, 점프·착지 | `JumoLocomotionComponent`, `fix_jumo_jump.py` |
| 캐릭터 임포트 | 선비·포졸 메시·스켈레톤·재질·애니메이션, 주모 애니메이션 추가 | `import_glb_characters.py`, `import_jumo.py` |
| 언덕 순찰 | 선비·포졸이 서로 다른 96개 경유점 경로를 반대 방향으로 반복 순찰 | `IslandPatrolCharacter`, `place_island_patrols.py` |
| 모바일 재질 | 선비·포졸 전용 BaseColor·Normal·MetallicRoughness 재질 연결 | `fix_mobile_character_materials.py` |
| 환경음 | 바람·파도 음원 반복 재생 | `generate_ambience.py`, `create_island_ambience.py` |
| 빌드·검증 | Mac 패키징, Android 전체 쿠킹, 지문 확인, 폴드 설치·순찰 로그 수집 | `build_mac_island.py`, `build_android_weather.py`, `install_patrol_fold.py` |

표의 C++ 클래스는 `Plugins/SimpleWeather/Source/SimpleWeather`에, 캐릭터 도구는 `Tools/JumoAnimations`에, 날씨·Android 빌드 도구는 `Tools/WeatherTest`에 있습니다.

## 날씨와 조작 UI

원본 비치맵을 복사한 테스트맵에 날씨 Actor와 노출 보정을 적용했습니다. 프로젝트의 시작 맵·게임모드 설정을 바꾸지 않고 복사본에서 조명과 비를 제어합니다.

10월 26일의 비는 큐브 메시 인스턴스 기반입니다. 최대 400개를 재사용하며 강수량 0/50/100%는 각각 0/200/400개에 대응합니다. 전용 반투명 Unlit 재질로 불투명한 빗줄기를 개선했고, 빗줄기 크기를 0.5×0.5×35cm로 줄였습니다. 낙하 속도는 700cm/s로 낮추고 비 영역이 플레이어를 따라가도록 수정했습니다. 물리적인 굴절·유체 시뮬레이션은 구현하지 않았습니다.

날씨 UI는 `AddToViewport(1000)`으로 모바일 기본 조이스틱보다 위에 배치합니다. 버튼 이외 빈 공간은 입력을 통과시킵니다. 화면 상단 중앙에 날씨 버튼과 강수량 슬라이더, 오른쪽에 달리기 ON/OFF와 점프 버튼을 둡니다.

달리기는 기존 `MaxWalkSpeed`를 저장한 뒤 1.6배로 전환하고 OFF 또는 위젯 제거 시 복원합니다. 점프 버튼의 누름·뗌은 `ACharacter::Jump/StopJumping`에 연결합니다.

## 주모 애니메이션

기존 주모 스켈레톤에 맞는 애니메이션 7개를 `/Game/Characters/JumoAnimations`에 가져왔습니다: `idle`, `walk`, `run`, `walk-casual`, `run-fast`, `wave`, `jump`.

런타임 `UJumoLocomotionComponent`가 이동 속도와 낙하 상태를 확인해 Idle/Walk/Run/Jump를 선택합니다. 점프는 반복하지 않으며 예상 체공 시간에 맞춰 재생 속도를 조절합니다. 착지하면 정지·이동 상태에 따라 Idle 또는 Walk/Run으로 돌아갑니다. 점프 클립에는 root lock을 적용해 CharacterMovement의 점프와 애니메이션의 상하 이동이 중복되지 않도록 했습니다.

원본 주모 Blueprint·스켈레톤·Animation Blueprint 에셋은 보존합니다. 추가 컴포넌트는 호환되는 스켈레톤과 클립이 있을 때만 동작하며, 테스트맵의 런타임 동작을 담당합니다. 초기 기록의 “달리기·점프 전용 애니메이션 미추가”는 이후 이 구현으로 보완됐습니다.

## 선비·포졸과 순찰

선비·포졸은 각각 전용 메시·스켈레톤·PhysicsAsset·재질·텍스처와 애니메이션 7개를 가져왔습니다. 에셋 경로는 `/Game/Characters/seonbi_final_rigged`, `/Game/Characters/pojol_final_rigged`입니다. GLB 임포트 경로와 애니메이션 바인딩 설정을 수정해 초기 임포트 실패를 해결했습니다.

모바일에서 텍스처가 표시되도록 각 캐릭터의 `Mobile` 폴더에 전용 재질과 BaseColor·Normal·MetallicRoughness 텍스처를 연결했습니다. 메시의 상대 회전을 Yaw -90도로 맞추고 캡슐 기준 높이를 보정했습니다.

순찰 중심은 `(900, 700)`cm이고, 선비 경로 반경은 `(2900, 3300)`cm, 포졸은 `(3150, 3550)`cm입니다. 각각 96개 경유점을 사용하며 선비는 155cm/s, 포졸은 145cm/s로 반대 방향을 걷습니다. 각 지점과 상하좌우 40cm 여유 지점의 지면 높이·경사를 검사했습니다.

`AIslandPatrolCharacter`는 CharacterMovement의 중력·바닥·충돌을 사용합니다. 플레이어 또는 AI 컨트롤러와 런타임 NavMesh 없이 저장된 경유점을 따라 이동하며 이동 중 Walk, 정지 시 Idle을 재생합니다. 맵에는 `Hill Patrol - Seonbi`, `Hill Patrol - Pojol` 두 Actor를 배치했습니다.

## 환경음

바람과 파도 음원을 생성·임포트해 날씨 Actor의 AudioComponent에서 반복 재생하도록 연결했습니다. 각 음원은 30.5초이며 에셋은 `/Game/Weather/Audio/S_Wind`, `/Game/Weather/Audio/S_Surf`입니다. 런타임 검사는 두 컴포넌트의 재생 상태를 확인하며, 실제 청취 품질을 자동으로 판정하지는 않습니다.

## 패키징과 원본 보존

Android 패키징에 프로젝트 네이티브 게임 타깃을 사용하도록 기본 게임 모듈과 Game/Editor 타깃을 추가했습니다. 새 반영 속성의 네이티브 코드와 이전 쿠킹 데이터를 혼합했을 때 발생한 시작 크래시를 해결하기 위해, Mac 쿠커 플러그인을 먼저 빌드하고 Android 데이터를 전체 재쿠킹하는 방식으로 변경했습니다.

완성된 APK의 SHA-256과 소스·맵·캐릭터 에셋 지문을 `Saved/WeatherAndroidPackage/full_cook_manifest.json`에 기록합니다. 설치 도구는 현재 소스와 일치하는 전체 쿠킹 APK만 허용합니다. 이전 네이티브 교체 APK는 `.apk.invalid`로 격리했습니다.

보호 대상은 원본 숲맵·비치맵·주모 Blueprint, 기존 Config 4개와 `.uproject`의 총 8개 파일입니다. `verify_protected.py`로 해시 일치를 검사했습니다. 날씨 섬 맵 변경 전 백업은 `Saved/JumoAnimations/MapBackups`에 보관합니다.

## 10월 26일 검증 결과

| 자료 | 확인된 결과 | 한계 |
| --- | --- | --- |
| `Saved/IslandMacValidation/20261026-225754/result.json` | 날씨 전환·강수량 0/50/100%, 주모 걷기·달리기·점프·착지, 캐릭터 자세·재질, 환경음 재생 상태 및 화면 9개 | 개별 실패 항목은 없지만 전체 `passed`는 `false`. 이 보고서를 전체 검증 성공으로 간주하지 않음 |
| `Saved/JumoAnimations/Device/20261026-230248/result.json` | 폴드8 `SM-F971N / R5KL7064NZV` 설치·실행, 두 캐릭터 시작, 선비 114개·포졸 99개 경유점 통과, `both_completed_loop: true` | 해당 시점 APK에 대한 결과이며 이후 변경본을 대신 검증하지 않음 |
| `Saved/JumoAnimations/ambience.json` | 바람·파도 에셋 각각 30.5초, 반복 재생 설정 | 청취 품질 판정 자료는 아님 |
| `Saved/JumoAnimations/mobile_materials.json` | 선비·포졸의 모바일 재질 및 텍스처 연결 | 실제 표현 품질은 화면 확인 필요 |

결과 파일과 APK·로그·화면은 로컬 `Saved` 아래의 생성물이며 Git 기본 추적 대상이 아닙니다. 당시 인수인계 문서의 “순찰 검증 중”이라는 설명보다 위 기기 최종 결과가 뒤에 생성됐습니다.

## 재현 명령

원본 프로젝트 콘텐츠와 UE 5.8.2가 있는 환경에서 프로젝트 루트 기준으로 실행합니다. 임포트·맵 배치 명령은 에셋을 구성할 때 사용하며, 이미 구성된 프로젝트를 빌드할 때는 빌드 명령부터 실행합니다.

```sh
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/import_jumo.py
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/import_glb_characters.py
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/fix_mobile_character_materials.py
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/fix_jumo_jump.py
python3 Tools/WeatherTest/build_and_generate.py --engine '/Users/Shared/Epic Games/UE_5.8' --platform Mac --build-only
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/place_island_patrols.py
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/create_island_ambience.py
python3 Tools/WeatherTest/verify_protected.py
python3 Tools/JumoAnimations/build_mac_island.py
python3 Tools/JumoAnimations/run_mac_validation.py
python3 Tools/WeatherTest/build_android_weather.py --engine '/Users/Shared/Epic Games/UE_5.8'
python3 Tools/JumoAnimations/install_patrol_fold.py
```

명령은 현재 도구 기준입니다. 현재 소스로 재현하면 10월 27일 후속 변경도 포함됩니다. 원본 GLB와 음원 등 입력 자료도 필요하며, 에디터 내부 `unreal` API 스크립트는 시스템 Python에서 직접 실행하지 않고 `run_unreal.py`로 실행합니다.

## 10월 27일 후속 변경과 구분

- Fab Weather System의 Niagara 비를 연결하고 기존 큐브 비를 비활성화했습니다. 강수량과 Humidity 연결, Mac 검증, Android APK 생성·폴드 실행을 진행했습니다. 상세 내용은 [WeatherSystem.md](WeatherSystem.md)를 참고합니다.
- 선비·포졸은 플레이어가 2.5m 이내로 접근하면 멈추고 플레이어를 바라보며, 3.5m 밖으로 나가면 순찰을 재개합니다. 이 반응은 10월 26일 구현에 포함되지 않습니다. 상세 설정과 검증은 [IslandCharacters.md](IslandCharacters.md)를 참고합니다.
- 최신 Mac 검증 `Saved/IslandMacValidation/20261027-085545/result.json`은 접근 반응과 순찰 완주를 포함해 `passed: true`입니다. 최신 APK도 폴드8에 설치·실행했습니다.

대사 데이터, 대화 UI, 선택지, 퀘스트 연계는 아직 구현하지 않았습니다.
