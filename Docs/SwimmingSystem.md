# 선녀 수면 수영

기존 선녀에 제자리 헤엄과 전진 수영, 조건부 수영 시작 버튼을 추가합니다. 잠수·산소·스태미나·수영 중 대시는 추가하지 않습니다. 기존 코드·에셋 경로의 `Jumo` 이름은 참조 보존을 위해 유지합니다.

## 동작

`UFairySwimmingComponent`가 실제 WaterBody의 수면과 지면 충돌을 조회합니다. 얕은 물에서는 걷고, 발이 물에 들어간 상태에서 깊이 100cm 이상이면 Unreal의 `MOVE_Swimming`으로 전환합니다. 조이스틱으로 방향을 바꾸고 전진하며, 놓으면 제자리 헤엄을 합니다. 깊이 70cm 이하의 해안으로 돌아오면 기본 이동으로 복귀합니다. 서로 다른 진입·이탈 깊이를 사용해 물가에서 상태가 반복 전환되는 것을 줄입니다.

깊이 80cm 이상이고 발이 물에 들어간 위치에서는 달리기 버튼 위에 `수영하기` 버튼이 나타납니다. 누르면 자동 전환 깊이에 도달하기 전에도 수영을 시작합니다. 육지·너무 얕은 물·대화 중에는 숨깁니다. 수영 중에는 같은 자리에 비활성 `수영 중` 표시를 사용합니다. 자동 수영은 그대로 유지합니다. 버튼은 기존 조이스틱과 달리기·점프 영역의 오른쪽 경계를 기준으로 배치합니다.

바다 렌더링 Actor는 물리 볼륨이 아니므로, 수영하는 선녀의 캡슐에만 임시 물리 볼륨을 지정합니다. 수영 중 수면 높이를 따라 수직 속도를 보정하고 native CharacterMovement가 이동·충돌을 처리합니다. 다른 NPC·섬 지형·바다 에셋에는 볼륨을 추가하거나 저장하지 않습니다. 물 밖으로 나오거나 외부 이동 잠금·컴포넌트 종료가 발생하면 기존 물리 볼륨 업데이트 정책, 부력, 수영 속도를 복구합니다. 외부 시스템이 설정한 이동 잠금은 해제하지 않습니다.

수영 중 달리기·점프 버튼은 비활성화하며 육지에서 이전 활성 상태로 돌아옵니다. 달리기 선택 상태와 육지 이동 속도는 유지합니다. 점프를 누른 채 입수했을 때는 점프 입력을 종료합니다.

컴포넌트의 `SwimSpeed`(180cm/s), `EnterDepth`(100cm), `ExitDepth`(70cm), `ManualStartDepth`(80cm)는 조정할 수 있습니다. 캡슐 중심의 수면 오프셋은 `TreadSurfaceOffset`(-55cm), `ForwardSurfaceOffset`(-25cm)로 조정합니다. `IsSurfaceSwimming()`·`CanStartSwimming()`·`GetSurfaceHeight()`를 Blueprint에서 조회하고 `StartSwimming()`으로 수동 진입할 수 있습니다. 수동 진입은 실제 물·깊이·입력 상태를 다시 확인합니다.

## 애니메이션과 원본 보존

현재 게임의 `Source/glb/jumo_final.glb_rigged.glb`를 FBX로 변환해 Mixamo에 `FAIRY_CURRENTGAME_MIXAMO`로 업로드했습니다. 이전 Mixamo 캐릭터의 동작을 사용하지 않았습니다. 현재 선녀에 적용한 `Treading Water`와 `Swimming`을 Without Skin, 30fps, Keyframe Reduction none으로 받았습니다.

- `/Game/Characters/JumoAnimations/Swimming/AS_Fairy_SwimIdle`: 제자리 헤엄, 3초.
- `/Game/Characters/JumoAnimations/Swimming/AS_Fairy_SwimForward`: 전진 수영, 약 4.53초.

두 클립은 기존 선녀 스켈레톤의 19개 뼈 이름과 비율로 베이크하고 애니메이션만 임포트했습니다. 이동은 CharacterMovement가 담당하므로 루트 위치 이동은 제거했습니다. 몸을 눕히는 회전은 애니메이션에 유지합니다. 기존 메시·스켈레톤·Animation Blueprint·맵은 덮어쓰지 않습니다. 원본 파일 해시는 `Saved/Swimming/Backups`에서 확인합니다.

Mixamo의 [Adobe 공식 FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html)는 개인·상업·비영리 게임에 캐릭터와 애니메이션을 royalty free로 사용할 수 있다고 안내합니다. 다운로드한 원본 FBX는 `Saved/Swimming/Mixamo`, 리타기팅 소스와 기록은 `Saved/Swimming/Retargeted`에 있으며 기존 방침대로 바이너리 에셋은 Git에 포함하지 않습니다.

## 재현과 검증

```sh
# Blender: 현재 선녀의 Mixamo 업로드 파일 생성
/Applications/Blender.app/Contents/MacOS/Blender --background --python Tools/Swimming/prepare_mixamo_character.py
# Mixamo에서 현재 선녀에 적용한 두 FBX를 Saved/Swimming/Mixamo에 보관한 뒤 실행
/Applications/Blender.app/Contents/MacOS/Blender --background --python Tools/Swimming/retarget_swimming.py
python3 Tools/JumoAnimations/run_unreal.py Tools/Swimming/import_swimming.py
python3 Tools/WeatherTest/build_and_generate.py --engine '/Users/Shared/Epic Games/UE_5.8' --platform Mac --build-only
python3 Tools/Swimming/run_validation.py
python3 Tools/WeatherTest/build_android_weather.py --engine '/Users/Shared/Epic Games/UE_5.8'
python3 Tools/WeatherTest/verify_weather_package.py
python3 Tools/Swimming/verify_originals.py
```

Mac 검증은 실제 섬의 얕은 물에서 깊은 바다로 이동해 native 수영 진입, 전진, 제자리 헤엄, 수면 유지, 외부 이동 잠금 복구, 해안 복귀와 점프, 수동 시작 버튼을 검사합니다. `Saved/Swimming/Validation/<시각>`에 로그·결과·화면 5장을 기록합니다. 임포트 도구는 기존 이름의 수영 클립이 있으면 덮어쓰지 않고 확인합니다. 리타기팅 변경을 반영할 때는 새 버전 경로를 사용하거나 기존 수영 클립만 명시적으로 재임포트해야 합니다.

`python3 Tools/Swimming/validate_fold_swimming.py`는 설치된 APK를 실행하고 펼친 화면에서 실제 ADB 터치로 버튼·조이스틱·해안 복귀와 이후 달리기·점프를 검사합니다. 결과는 `Saved/Swimming/Android/<시각>`에 저장합니다. 설치 자체는 먼저 `adb install -r`로 수행합니다.

**개발 검증 주의:** UE의 `BugItGo`는 위치만 바꾸는 명령이 아니라 Ghost 모드(비행·충돌 해제)도 켭니다. 검증 도구는 이 명령 직후 반드시 `Walk`를 실행해 충돌·중력과 정상 이동을 복원합니다. 이 복원을 빠뜨린 첫 Android 검사에서는 수면에 서 있는 현상이 발생했습니다. 실제 수영 조건의 오류와 구분해야 하며, Ghost 상태의 결과를 수영 검증으로 사용하지 않습니다.

진입·이탈 로그는 `FAIRY_SWIM_ENTER`, `FAIRY_SWIM_EXIT`, 애니메이션 상태는 `JUMO_LOCOMOTION_STATE: SwimIdle/SwimForward`입니다. APK에는 두 클립을 기존 이동 컴포넌트의 참조를 통해 포함하며, 전체 쿠킹 지문에도 새 애니메이션을 포함합니다.

## 결과

2026-10-27 Android ASTC 전체 쿠킹·패키징 성공(UAT ExitCode 0). APK는 `Saved/WeatherAndroidPackage/Android_ASTC/buildtest-arm64.apk`, SHA-256은 `c8c85c27088713a9358591a5ae529e9a4a3a8a1988d4ec8dd3f342e6918feedd`입니다. 현재 소스·애니메이션과 패키지 지문 일치를 확인했고 폴드 `R5KL7064NZV` / `SM-F971N`에 업데이트 설치 `Success`, GameActivity 실행 `Status: ok`를 확인했습니다.

| 검사 | 결과·근거 |
| --- | --- |
| Mac 실제 실행 | `Saved/Swimming/Validation/20261027-142633/result.json` 통과. 에디터의 `-game` 모드에서 자동·수동 수영, 수면 유지, 자연스러운 해안 복귀, 이동 잠금 정리와 점프를 확인했습니다. |
| 폴드 펼친 화면 | 실제 2448×1848 화면에서 수영 버튼 표시·터치 시작, 자동 진입, 전진·제자리 헤엄, 수영 중 달리기·점프 비활성화를 확인했습니다. |
| 폴드 해안 조작 복구 | `Saved/Swimming/ShoreControls/20261027-144252/result.json` 통과. 실행 중인 수영 세션에서 개발 위치 이동으로 얕은 해안에 복귀한 뒤 실제 점프·달리기 터치를 확인했습니다. 자연스러운 해안 횡단 검사와는 별도입니다. |
| 폴드 연속 자동 터치 검사 | **전체 미통과.** `Saved/Swimming/Android/20261027-144314/result.json`은 해안 복귀 시간 초과입니다. 이전 `20261027-142841` 실행에서는 실제 조이스틱 해안 복귀가 확인됐으나 이후 앱의 홈 화면 전환으로 후속 검사가 중단됐습니다. 연속 시나리오 전체 성공으로 표시하지 않습니다. |
| 원본 보존 | `verify_originals.py`: 선녀 원본 에셋·GLB·날씨 섬 맵 12개 해시 유지. 기존 보호 검사 8개도 통과했습니다. |

기기 자동화에서 일부 연속 조이스틱 제스처가 가속도 0으로 관측돼, 연속 해안 복귀 검사의 재현성은 추가 확인이 필요합니다. 별도 실제 입력 조회에서는 수영 가속도 1500cm/s²와 약 180cm/s 이동을 확인했습니다. Android의 기본 ADB `tap`은 누름·해제가 같은 게임 프레임에 들어가 점프가 생략되는 경우가 있어, 터치 검사는 180ms 누름으로 보정했습니다. 이 작업에서 커버 화면, 잠수, 독립 Mac 패키징과 기존 날씨·대화·순찰 전체 회귀 검사는 수행하지 않았습니다. 기존 기능과 에셋을 제거하거나 덮어쓰지 않았습니다.

별도 캡처 폴더는 `Screenshots/SwimmingSystem/20261027-144507`입니다. Mac 5장, 폴드 수영 7장, 별도 해안 조작 복구 1장과 원본 보고서를 모았습니다. 각 보고서의 성공·실패 상태를 그대로 보존했습니다. 소스·대화 데이터·문서는 Git에 포함하며, 맵·캐릭터·애니메이션·APK·실행 캡처와 로컬 개발 토큰은 별도 보관합니다. UI 실행에 필요한 원본 궁서 폰트는 배포 고지와 함께 포함합니다.

## 수정·추가 파일

- 신규 런타임: `Plugins/SimpleWeather/Source/SimpleWeather/Public/FairySwimmingComponent.h`, 대응 `Private/FairySwimmingComponent.cpp`.
- 신규 실행 검사: 같은 모듈의 `Public/SwimmingValidationActor.h`, `Private/SwimmingValidationActor.cpp`.
- 이동·UI 연결: 같은 모듈의 `Public/JumoLocomotionComponent.h`, `Private/JumoLocomotionComponent.cpp`, `Private/SimpleWeatherActor.cpp`, `Public/WeatherControlWidget.h`, `Private/WeatherControlWidget.cpp`, `Private/DialogueWidget.cpp`.
- Water 의존성: `Plugins/SimpleWeather/SimpleWeather.uplugin`, `Plugins/SimpleWeather/Source/SimpleWeather/SimpleWeather.Build.cs`.
- 신규 애니메이션: `Content/Characters/JumoAnimations/Swimming/AS_Fairy_SwimIdle.uasset`, `AS_Fairy_SwimForward.uasset`(로컬 바이너리).
- 제작·검증 도구: `Tools/Swimming/prepare_mixamo_character.py`, `upload_mixamo.py`, `retarget_swimming.py`, `import_swimming.py`, `run_validation.py`, `validate_fold_swimming.py`, `validate_shore_controls.py`, `inspect_fold_swimming.py`, `inspect_swim_input.py`, `verify_originals.py`, `export_screenshots.py`.
- 기존 위치 이동 도구 복구: `Tools/DialogueTest/validate_fold_dialogue.py`, `Tools/JumoAnimations/preview_patrol_device.py`.
- 문서: `Docs/SwimmingSystem.md`, `README.md`.
