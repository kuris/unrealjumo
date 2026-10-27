# 섬 캐릭터와 언덕 순찰

`Source/glb`의 주모·선비·포졸을 프로젝트로 가져왔습니다.

| 캐릭터 | 콘텐츠 브라우저 경로 | 가져온 내용 |
| --- | --- | --- |
| 주모 | `/Game/Characters/JumoAnimations` | 기존 주모 스켈레톤용 애니메이션 7개 |
| 선비 | `/Game/Characters/seonbi_final_rigged` | 메시, 전용 스켈레톤, PhysicsAsset, 재질 1개, 텍스처 3개, 애니메이션 7개 |
| 포졸 | `/Game/Characters/pojol_final_rigged` | 메시, 전용 스켈레톤, PhysicsAsset, 재질 1개, 텍스처 3개, 애니메이션 7개 |

각 파일의 애니메이션은 `idle`, `walk`, `run`, `walk-casual`, `run-fast`, `wave`, `jump`입니다. 길이가 모두 양수이고 각 캐릭터의 스켈레톤을 참조하는지 검사한 뒤 저장했습니다. 기존 주모 메시·스켈레톤·ABP는 유지했습니다. 주모의 기존 ABP는 아직 Idle/Walk를 사용합니다.

`/Game/Maps/LV_BeachIsland_Weather`에 `Hill Patrol - Seonbi`와 `Hill Patrol - Pojol`을 추가했습니다. 아웃라이너 폴더는 `Island/Hill Patrols`입니다. 두 캐릭터는 중앙 언덕군 주위의 서로 다른 타원 경로를 반대 방향으로 반복해서 걷습니다. 중앙 좌표는 `(900, 700)`cm이며, 선비 경로 반경은 `(2900, 3300)`cm, 포졸은 `(3150, 3550)`cm입니다. 안쪽 경로는 급경사가 있어 피했습니다.

`AIslandPatrolCharacter`는 CharacterMovement로 중력·바닥·충돌을 처리합니다. 각 경로의 96개 지면 좌표를 저장했고, 각 좌표와 상하좌우 40cm 여유 지점에서 마른 지면과 법선 Z ≥ 0.8을 검사했습니다. 속도는 선비 155cm/s, 포졸 145cm/s입니다. 이동 중 `walk`, 정지 시 `idle`을 반복 재생합니다. 경로는 마지막 점에서 첫 점으로 이어지며, 플레이어 컨트롤러를 사용하지 않습니다.

원본 비치맵·숲맵·주모·Config·uproject의 보호 해시 8개가 일치합니다. 수정 전 날씨 섬 맵은 `Saved/JumoAnimations/MapBackups`에 보관했습니다.

## 플레이어 접근 반응

선비·포졸은 플레이어가 수평 거리 250cm 이내, 높이 차 150cm 이내에
들어오면 걷기를 멈추고 idle을 재생합니다. 초당 최대 240도로 몸을 돌려
플레이어를 바라보고, 플레이어가 주변을 이동하면 방향을 계속 따라갑니다.
거리가 350cm를 넘거나 높이 차가 범위를 벗어나거나 플레이어가 사라지면
저장된 다음 경유점으로 순찰을 재개합니다. 멈춰 있는 동안 경유점을 넘기지 않습니다.

캐릭터 Details의 `Island Patrol > Conversation`에서 `Approach Distance`,
`Resume Distance`, `Max Interaction Height`, `Face Player Turn Speed`를 조절합니다.
`IsFacingPlayer()`는 Blueprint에서도 조회할 수 있습니다.
런타임 로그는 `ISLAND_PATROL_ATTENTION: <이름> face player / resume patrol`입니다.
대사·대화 UI는 아직 추가하지 않았습니다.

Mac 실행 검증은 두 캐릭터 각각 접근 시 정지·방향 전환, 300cm에서 정지 유지,
플레이어 이동 시 방향 추적, 이탈 시 이동 재개를 검사한 뒤 순찰 완주를 확인합니다.

2026-10-27 검증 통과:
`Saved/IslandMacValidation/20261027-085545/result.json`은 `passed: true`, 종료 코드 0,
실패 항목 없음입니다. 두 캐릭터의 접근 반응과 반응 후 한 바퀴 순찰 완주를 확인했고
화면 11개를 저장했습니다. Android ASTC 전체 재쿠킹 APK도 지문 검사를 통과했으며,
폴드8 업데이트 설치 `Success`와 GameActivity 실행 `Status: ok`를 확인했습니다.
기기 실행 화면은 `Saved/PatrolAttentionValidation/android_launch.png`입니다.
접근 반응의 자동 동작 검증은 Mac에서 수행했습니다.

## 재현 명령

프로젝트 루트의 Mac 터미널에서 실행합니다. 에셋 임포트 도구는 목적지에 에셋이 있으면 덮어쓰지 않고 검사합니다. 배치 도구는 기존 `Hill Patrol` Actor를 갱신하며 날씨 섬 맵을 저장합니다.

```bash
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/import_jumo.py
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/import_glb_characters.py
python3 Tools/WeatherTest/build_and_generate.py --engine '/Users/Shared/Epic Games/UE_5.8' --platform Mac --build-only
python3 Tools/JumoAnimations/run_unreal.py Tools/JumoAnimations/place_island_patrols.py
python3 Tools/WeatherTest/build_android_weather.py --engine '/Users/Shared/Epic Games/UE_5.8'
python3 Tools/JumoAnimations/install_patrol_fold.py
```

마지막 도구는 전체 쿠킹 APK의 소스·맵·캐릭터 에셋 지문과 APK 해시를 검사하고 `R5KL7064NZV`에 설치·실행합니다. 최대 240초 동안 두 순찰의 진행 로그를 확인하며 화면을 저장하고, 두 캐릭터가 각각 96개 경유점을 통과해야 성공합니다. 개발 빌드 콘솔의 `BugItGo`로 플레이어를 선비 근처로 이동시켜 현장 확인을 돕습니다. 앱은 실행된 상태로 둡니다.

결과 파일: `Saved/JumoAnimations/import.json`, `characters_import.json`, `patrol_placement.json`, `Device/<시각>/result.json`. 기기 런타임 검증 결과는 빌드 완료 후 이 문서에 갱신합니다.
