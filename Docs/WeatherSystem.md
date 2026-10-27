# Fab Weather System 적용

제품 링크: https://fab.com/s/280c126dbc5f

다운로드 원본은 `WeatherSystem/WeatherSystem.uproject`, 가져온 콘텐츠는
`Content/WeatherSystem`입니다. 원본의 `/Game/WeatherSystem` 경로를 유지했습니다.

적용 맵은 `/Game/Maps/LV_BeachIsland_Weather`입니다. `WeatherManagmentBP`를
배치하고 기존 `SimpleWeatherActor.WeatherSystemActor`에 연결합니다. 기존 버튼과
강수량 슬라이더가 블루프린트의 `SetRainComponentState`, `SetRain`, `SetHumidity`를
호출합니다. 플레이어 주변 비 배치는 에셋의 `SetRainLocation`이 처리합니다.
에셋 자체 Q키 입력은 비활성화하고 기존 R키와 터치 UI를 사용합니다.

기존 큐브 빗줄기는 이 맵에서 제거합니다. 조명, 바다, 주모, 선비·포졸 순찰,
달리기·점프, 바람·파도 소리는 기존 구성을 사용합니다. 원본 비치 맵과 프로젝트
시작 맵 설정은 변경하지 않습니다.

표면 젖음은 에셋의 `GeneralMPC.Humidity`에 강수량을 전달합니다. 기존 섬 지형과
오브젝트의 재질을 에셋 데모 재질로 교체하지 않았으므로, 이 값만으로 모든 기존
표면에 물웅덩이·물방울 효과가 추가되는 것은 아닙니다.

적용 도구:

```sh
python3 Tools/JumoAnimations/run_unreal.py Tools/WeatherTest/apply_weather_system.py
```

도구는 날씨 맵을 `Saved/WeatherSystem/MapBackups`에 백업하고, 실제 Niagara
`User.SpawnRateMultiplyer`, 표시 상태, `GeneralMPC.Humidity`를 0/50/100% 및
맑음 복귀에서 검사한 뒤 저장합니다. 결과는
`Saved/WeatherSystem/application_report.json`입니다.

Mac 패키지와 실행 검증:

```sh
python3 Tools/JumoAnimations/build_mac_island.py
python3 Tools/JumoAnimations/run_mac_validation.py
```

검증 도구는 실제 UI 버튼·슬라이더를 호출하고 Niagara와 Humidity를 검사합니다.
Mac 결과는 `Saved/IslandMacValidation/latest.txt`가 가리키는 폴더의 `result.json`,
`runtime.log`, PNG 화면에 기록됩니다.

## 2026-10-27 실행 검증

Mac 패키지 실행 검증 통과: `Saved/IslandMacValidation/20261027-084335/result.json`
(`passed: true`, 종료 코드 0, 실패 항목 없음, 화면 9개).
UI 이벤트를 호출해 강수량 0/50/100%의 Niagara 값·표시 상태·Humidity와
맑음 복귀를 검사했습니다. 실제 마우스·터치 입력 검증과는 별개입니다.
`08_rain_100.png`에서 가는 빗줄기 렌더링을 확인했습니다.
주모의 걷기·달리기·점프·착지, 바람·파도 오디오 재생 상태,
선비·포졸 각각 96개 경유점의 순찰 완주도 통과했습니다.
원본 보호 대상 8개 파일의 해시가 일치합니다.

Fab Weather System을 포함한 Android ASTC 전체 빌드·쿠킹·패키징이
성공했습니다. APK는 `Saved/WeatherAndroidPackage/Android_ASTC/buildtest-arm64.apk`이며
소스 지문·APK 해시 검사를 통과했습니다.
폴드8(`SM-F971N`, `R5KL7064NZV`) 업데이트 설치 `Success`, GameActivity 실행
`Status: ok`를 확인했습니다. 게임을 전면에 둔 상태이며
`Saved/JumoAnimations/Device/20261027-084736/launch_screen.png`에서
비 100%의 실제 빗줄기, 날씨·강수량·달리기·점프 UI와 캐릭터를 확인했습니다.
두 순찰 시작 로그도 확인했습니다. Android 순찰 완주 수집은 ADB 연결 오류로
중단됐으므로 이번 APK의 기기 완주 검증은 미완료입니다.
