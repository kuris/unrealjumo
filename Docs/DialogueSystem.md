# NPC 대화 시스템

대상: UE 5.8.2, `buildtest`, `/Game/Maps/LV_BeachIsland_Weather`, Mac Apple Silicon / Android ASTC.

## 플레이 방법

1. 선비 또는 포졸의 수평 거리 250cm 이내로 다가갑니다. 기존 접근 로직이 NPC를 멈추고 플레이어 쪽으로 회전시키며, 가장 가까운 대화 가능한 NPC 머리 위에 말풍선과 화면 오른쪽에 금색 `대화` 버튼이 표시됩니다.
2. `대화` 버튼을 누릅니다. 데스크톱에서는 E키도 사용할 수 있습니다. 0.4초 동안 임시 대화 카메라로 전환하고 하단에 이름·대사·선택지를 표시합니다.
3. 선택지를 누르면 해당 노드로 분기합니다. 선비는 이상한 소리/주막, 포졸은 낯선 사람/목격담을 각각 설명합니다. 답변에서는 종료하거나 첫 대사로 돌아갈 수 있습니다.
4. X, 종료 선택지 또는 Esc로 닫습니다. 카메라가 0.4초 동안 원래 ViewTarget으로 복귀하고 이동·점프·시점 입력과 기존 UI가 복구됩니다.

대화 버튼은 250cm 밖에서 숨지만, NPC는 기존 350cm 이탈 기준까지 플레이어를 바라봅니다. 대화를 닫아도 플레이어가 근처에 있으면 NPC는 기존 접근 상태를 유지합니다. 350cm 밖으로 이동하면 저장된 다음 경유점으로 순찰을 재개합니다. 높이 차는 기존 `MaxInteractionHeight`를 따릅니다. 공중에서는 대화 시작을 허용하지 않습니다.

화면 밖이거나 숨겨진 NPC, 플레이어의 몸이나 다른 충돌체에 가려진 NPC에는 대화 버튼을 표시하지 않습니다. NPC의 머리·상체가 화면 안에 있는지 투영하고 카메라에서 두 지점으로 시야를 검사합니다. 거리 내에 있으나 보이지 않는 NPC 때문에 빈 화면에 버튼만 나타나는 상황을 방지합니다.

## 구성

| 파일 / 클래스 | 역할 |
| --- | --- |
| `Plugins/SimpleWeather/Data/Dialogues.json` | NPC별 이름, 첫 노드, 대사, 선택지 및 다음 노드 |
| `UDialogueDataAsset` | Blueprint에서 편집 가능한 대화 구조, JSON 로더 및 유효성 검사 |
| `UDialogueComponent` | NPC의 데이터 연결과 대화 세션 소유권 |
| `UDialogueManagerComponent` | 로컬 플레이어의 가장 가까운 대상 선택, 분기·카메라·입력·종료 정리 |
| `UDialogueWidget` | 대화 버튼·머리 위 말풍선·한지색 패널·먹색 이름표·금색 선택지 |
| `DialogueValidation.cpp` | 기존 섬 실행 검증에 대화·종료·복구 시나리오 추가 |

C++ 파일은 `Plugins/SimpleWeather/Source/SimpleWeather/Public`, `Private`에 있습니다. 순찰 NPC는 기본 서브오브젝트로 `UDialogueComponent`를 갖습니다. `ASimpleWeatherActor::SetupPlayerLocomotion`은 기존 주모 동작 컴포넌트와 함께 로컬 플레이어에 대화 관리자 컴포넌트를 등록합니다. 맵·메시·스켈레톤·Animation Blueprint 에셋을 덮어쓰거나 다시 저장할 필요가 없습니다.

NPC 정지·바라보기는 `AIslandPatrolCharacter::UpdatePlayerAttention` 한 곳에서 처리합니다. 관리자는 `IsFacingPlayer()`와 접근 거리로 버튼 표시 대상을 선택하며, 별도의 순찰 타이머를 만들지 않습니다. 대화 중에는 NPC의 대화 세션이 기존 접근 상태를 유지하도록 합니다. 이동 중이던 경유점 인덱스는 유지됩니다.

## 대화 데이터 편집

기본 대사는 C++ 문자열에 들어 있지 않습니다. `Data/Dialogues.json`을 수정하고 다시 패키징합니다. `.Build.cs`의 `RuntimeDependencies`가 JSON을 UFS 파일로 포함하므로 Android에서도 패키지 내부 데이터를 읽습니다. APK 소스 지문에도 JSON을 포함합니다.

```json
{
  "id": "new_npc",
  "name": "새 인물",
  "entry": "greeting",
  "nodes": [
    {
      "id": "greeting",
      "text": "어서 오시오.",
      "choices": [
        {"id": "ask", "text": "이곳은 어디인가요?", "next": "answer"},
        {"id": "leave", "text": "다음에 올게요.", "end": true}
      ]
    },
    {
      "id": "answer",
      "text": "바닷바람이 머무는 섬이오.",
      "choices": [{"id": "thanks", "text": "고맙습니다.", "end": true}]
    }
  ]
}
```

새 레코드를 최상위 `npcs` 배열에 추가하고 NPC의 `Dialogue.DialogueId`를 해당 `id`로 설정합니다. `DialogueId`가 비어 있으면 기존 `PatrolName`을 사용합니다. 기존 선비·포졸은 `seonbi`, `pojol`로 자동 연결됩니다. `Dialogue.Data`에 직접 만든 `UDialogueDataAsset`을 지정하면 JSON 카탈로그보다 우선합니다.

노드마다 선택지 1~3개를 사용합니다. `next`는 같은 NPC의 유효한 노드 ID를 가리키며 `end: true`는 대화를 종료합니다. NPC·노드·선택지 ID 중복과 존재하지 않는 다음 노드를 검사합니다. 잘못된 데이터는 오류 로그를 남기고 대화를 시작하지 않습니다. 기존 날씨·주모 동작·순찰은 계속 사용할 수 있습니다.

## 카메라와 입력 보존

### 공통 대화 배치 프로필

`Dialogues.json` 최상위 `presentation`에서 모든 NPC의 대화 구도를 관리합니다. 개별 NPC 레코드에도 `presentation`을 넣어 필요한 값만 덮어쓸 수 있습니다. Data Asset을 직접 쓰면 `Presentation` 필드를 편집합니다.

| 값 | 기본값 | 의미 |
| --- | --- | --- |
| `playerDistanceCm` | 200 | NPC 앞 플레이어 배치 거리 |
| `cameraDistanceCm` | 340 | NPC 기준 플레이어 방향 카메라 거리 |
| `cameraSideOffsetCm` | -260 | 음수이면 주모를 왼쪽 앞, NPC를 오른쪽에 배치 |
| `cameraHeightCm` | 110 | NPC 캡슐 중심 기준 카메라 높이 |
| `focusHeightCm` | 50 | 두 인물 사이 주시점의 높이 |
| `fieldOfView` | 60 | 대화 카메라 FOV |
| `stagePlayer` | true | 안전한 지면에서 플레이어를 정해진 거리·방향으로 배치 |
| `restorePlayerTransform` | true | 종료 시 플레이어의 원래 위치·회전 복원 |

NPC의 현재 위치를 기준점으로 사용하므로 맵에 고정 좌표를 추가하거나 순찰 경로를 수정하지 않습니다. 플레이어 배치 전 지면·경사와 캡슐 충돌·이동 경로를 검사합니다. 안전한 배치가 불가능하면 현재 위치에서 대화하고, 카메라는 같은 좌우 구도 규칙을 사용합니다. NPC별 상대 배치 값을 한 곳에서 관리해 이후 인물·대사를 늘려도 동일한 구도를 유지할 수 있습니다.

- 기존 카메라 컴포넌트·SpringArm 설정을 수정하지 않고 세션 동안 `ACameraActor`를 사용합니다. NPC·플레이어의 위치와 기존 카메라 방향으로 구도를 정하고 Camera 채널 구체 스윕으로 장애물과 겹치지 않도록 보정합니다.
- 원래 ViewTarget, ControlRotation, 마우스 커서 상태, Pawn 입력 활성화 여부와 이동 모드를 저장합니다. 대화 중 Pawn 입력과 이동 모드를 막고 이동·시점 무시 카운터를 각각 한 번 추가합니다. 종료 시 추가한 카운터만 해제하므로 다른 시스템의 기존 잠금은 유지됩니다.
- 날씨·달리기·점프 위젯은 이전 표시·활성 상태를 저장한 뒤 숨기고 비활성화합니다. 실제 Slate 조이스틱의 표시 상태도 저장·복구하며 TouchInterface 설정을 교체하지 않습니다. 달리기 ON/OFF 상태와 기존 속도는 보존합니다.
- X, 선택지 종료, NPC 제거, 위젯 제거, 맵 이동, 컴포넌트 종료와 앱 백그라운드 전환은 같은 종료 정리를 사용합니다. 정리는 중복 호출에도 안전하도록 처리합니다. 레벨 전환·제거 시에는 카메라를 즉시 복원합니다.

대화 중 날씨 버튼과 R키 전환을 막고, 날씨 Actor·Niagara·강수량·환경음 자체는 계속 동작시킵니다. 종료 후 기존 조작을 다시 사용할 수 있습니다.

모바일 원본 `UI_TouchSimple`의 왼쪽 이동·오른쪽 시점 조이스틱도 대화 중 함께 숨기고 비활성화합니다. 오른쪽 시점 조이스틱 위에 있던 네모는 원본 `Btn_Jump`이며 새 점프 버튼과 중복돼 런타임에서 숨깁니다. 원본 위젯 에셋은 수정하지 않습니다. 달리기·점프는 왼쪽 이동 조이스틱의 실제 위젯 오른쪽 경계에 나란히 배치하고, 공간이 좁으면 세로로 배치합니다. 날씨 위젯이 제거될 때 원본 점프 버튼의 표시·활성 상태도 복구합니다.

## UI와 화면 크기

SafeZone 안의 Canvas와 DPI Scaling을 사용합니다. 하단 패널은 가용 높이의 약 39%를 기본으로 하고, 작은 창에서는 가독성을 위해 최소 높이를 적용합니다. 패널 너비는 최대 1000 Slate 단위로 제한합니다. 좁거나 낮은 화면에서 내용이 넘치면 대사·선택지 영역을 세로로 스크롤할 수 있습니다. 선택지는 최소 높이 48 Slate 단위, 대화 버튼은 180×64입니다.

한지색 둥근 패널, 투명한 둥근 그림자, 먹색 이름표, 금색 강조 버튼은 UMG 도형으로 그립니다. 그림자는 패널 뒤의 별도 Canvas 자식으로 배치해 내용 위에 그려지지 않도록 합니다. 외부 UI 이미지·유료 플러그인·유료 에셋을 추가하지 않습니다. 첨부 이미지의 인물 초상화와 미니맵은 이번 대화 기능에 추가하지 않았습니다. 기존 맵과 캐릭터 표현을 유지합니다.

대화와 기존 조작 UI의 한글은 공식 배포처에서 받은 **조선궁서체**를 사용합니다. 배포처는 [조선일보 서체 안내](https://event.chosun.com/100/100font.html), 원본은 [ChosunGs.zip](https://fontdown.chosun.com/100/ChosunGs.zip)입니다. 개인·기업 무료 사용과 원본 재배포 조건에 따라, 원본 TTF를 수정하지 않고 `Plugins/SimpleWeather/Data/Fonts/ChosunGs.TTF`에 포함합니다. 배포처의 고지와 다운로드 주소·원본 SHA-256은 같은 폴더의 `LICENSE-Chosun.txt`에 기록하고 게임 패키지에도 포함합니다. 이 글꼴은 SIL OFL이 아닌 제작사 무료 배포 조건을 따릅니다. 폰트 자체를 별도 판매하거나 변형하지 않습니다.

## 확장 지점

`OnChoiceSelected(NpcId, ChoiceId, EventTag)`, `OnNodeChanged(NpcId, NodeId)`, `OnConversationEnded(NpcId)`를 Blueprint 또는 C++에서 구독할 수 있습니다. 선택지 JSON에 선택적인 `event` 키를 넣으면 `EventTag`로 전달합니다. 퀘스트·호감도·아이템 지급·음성 기능은 현재 구현하지 않았으며 이벤트를 연결할 수 있도록 열어 두었습니다.

## 빌드와 검증

```sh
python3 Tools/WeatherTest/verify_protected.py
python3 Tools/JumoAnimations/build_mac_island.py
python3 Tools/JumoAnimations/run_mac_validation.py
python3 Tools/WeatherTest/build_android_weather.py --engine '/Users/Shared/Epic Games/UE_5.8'
python3 Tools/JumoAnimations/install_patrol_fold.py
python3 Tools/DialogueTest/validate_fold_dialogue.py --label unfolded
```

Mac 실행 검증은 `-IslandValidate -DialogueValidate`를 사용합니다. 가까운 거리에서 버튼 표시, 250cm 밖 숨김과 350cm 접근 상태 유지, 두 NPC의 두 분기·종료 선택지·X, 카메라·입력 복원, 여러 NPC 중 가장 가까운 대상, NPC 삭제·위젯 제거, 기존 입력 잠금과 숨긴 UI 보존을 검사합니다. 대화 종료 후 날씨 전환·Niagara 값·달리기·점프·착지와 두 NPC 순찰 완주도 검사합니다.

주요 로그는 `DIALOGUE_READY`, `DIALOGUE_CANDIDATE`, `DIALOGUE_STARTED`, `DIALOGUE_NODE`, `DIALOGUE_CHOICE`, `DIALOGUE_ENDED`입니다. Mac 보고서·스크린샷은 `Saved/IslandMacValidation/latest.txt`가 가리키는 폴더에 있습니다. Android는 APK 생성만으로 터치 조작 검증을 통과한 것으로 보지 않습니다.

Android 대화 검증 도구는 APK 지문을 검사하고 실제 ADB 터치 이벤트로 대화 시작·선택지·X·종료 선택지·기존 날씨/달리기/점프를 조작합니다. 개발 빌드의 `DIALOGUE_UI_RECTS`가 DPI·SafeZone 적용 후의 렌더 뷰포트 좌표와 뷰포트 크기를 제공합니다. 도구는 모바일 렌더 해상도와 실제 PNG 화면 해상도의 비율로 물리 터치 좌표를 변환합니다. 대화 버튼·말풍선·대사·선택지가 화면과 패널 안에 들어가는지, 버튼이 서로 겹치지 않는지, 달리기·점프가 왼쪽 조이스틱 옆에 있는지 검사합니다. 스크린샷과 로그·선택지 결과는 `Saved/DialogueSystem/Android/<시각>`에 남깁니다. NPC에 접근할 때만 개발 빌드 콘솔로 플레이어를 이동하며, 대화 조작은 C++ 이벤트 직접 호출 대신 실제 Android 터치를 사용합니다. 커버 화면은 사용자 요청에 따라 이번 실기기 검증에서 제외하고 펼친 화면을 대상으로 합니다.

## 이번 작업의 검증 상태

아래는 2026-10-27 오전 대화 구현 시점의 기록입니다. 이후 수영·모바일 조작 변경을 포함한 APK 설치와 검증 결과는 [SwimmingSystem.md](SwimmingSystem.md)의 결과 절을 참고하세요.

2026-10-27 11:03 기준 Android ASTC 전체 쿠킹·패키징이 성공했습니다(UAT ExitCode 0). 결과는 `Saved/WeatherAndroidPackage/Android_ASTC/buildtest-arm64.apk`이며, `verify_weather_package.py`로 현재 소스·대화 데이터·폰트와 패키지 지문이 일치함을 확인했습니다. APK SHA-256은 `82b43943214fad2ef2d08105c44225b473a19b835f67644cabd5cac4575f21c0`입니다.

최종 APK에는 실제 이동 조이스틱 이름 `Thumbstick_Move`에 맞춘 위치 계산, 중복 `Btn_Jump` 숨김, 달리기 터치 처리 변경과 입력 진단 로그가 포함됩니다. 사용자의 “APK까지만” 요청에 따라 이 버전의 설치·실행은 진행하지 않았습니다. 따라서 달리기·점프의 실기기 동작, 최종 버튼 겹침·메시지 위치는 검증 완료로 표시하지 않습니다. 커버 화면도 검증하지 않았습니다.

Mac 패키징과 이전 버전 실행 검사는 수행했습니다. `Saved/IslandMacValidation/20261027-103143` 보고서는 통과했지만, 이후 구도 변경 버전의 `20261027-103906` 보고서는 시점 이탈 검사 2건이 실패했습니다. 검사 카메라를 수정했으나 최종 코드의 Mac 실행 재검증은 남아 있습니다. Android 이전 실행 검사도 터치 좌표 변환 및 원본 위젯 탐색 문제로 전체 통과하지 못했습니다. 해당 원인을 수정한 최종 APK의 실기기 재검증이 필요합니다.

별도 캡처 폴더 `Screenshots/DialogueSystem/20261027-110200`에 Mac 20장과 폴드8 펼친 화면 3장을 모았습니다. 모두 최종 조이스틱·터치 수정 전의 실행 자료이며 최종 APK 검증 화면은 아닙니다.

변경 전 보호 파일 8개와 날씨 섬 맵을 `Saved/DialogueSystem/Backups/20261027-100349`에 백업했습니다. 백업의 `manifest.json`에 원본 해시를 기록했습니다. Git에 이미 존재하던 개발 토큰·로컬 보호 기준의 차이는 이번 기능 수정에 포함하지 않습니다. 에셋·빌드 산출물은 기존 방침대로 Git에서 제외합니다.
