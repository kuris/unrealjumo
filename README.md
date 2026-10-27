# UE 5.8 Simple Weather

기존 buildtest 프로젝트에 추가하는 맑음·비 전환 플러그인과 해변 테스트맵입니다. 화면 최상단 터치 버튼과 R키로 날씨를 전환하고 슬라이더로 강수량을 조절합니다. 날씨 섬에는 Fab Weather System의 Niagara 비를 연결했습니다.

Mac arm64 플러그인 빌드와 Fab Weather System을 포함한 Mac·Android ASTC 패키징이 성공했습니다. 선비·포졸의 언덕 주변 순찰과 주모의 걷기·달리기·점프를 구현했습니다. Mac 실행 검증을 통과했고, 최신 APK를 폴드8에 설치·실행해 비 100% 화면을 확인했습니다.

- [캐릭터 에셋·언덕 순찰](Docs/IslandCharacters.md)
- [2026-10-26 구현 보고서](Docs/ImplementationReport_2026-10-26.md)
- [현재 상태·실행 명령·인수인계](Docs/WeatherWorkHandoff_2026-10-26.md)
- [Fab Weather System 적용·Mac 검증](Docs/WeatherSystem.md)
- [맵 생성·노출 보정 도구 설명](Docs/WeatherTest.md)

이 자료는 원래 buildtest 프로젝트에 추가한 소스·도구·문서·테스트맵을 대상으로 합니다. 원본 프로젝트 콘텐츠가 필요하며 APK·엔진·빌드 캐시는 포함하지 않습니다. 프로젝트에 Plugins/SimpleWeather, Source, Tools/WeatherTest와 새 맵을 함께 적용해야 Android 네이티브 빌드를 재현할 수 있습니다.

Android 재배포 시 Mac 쿠커 플러그인과 Android 데이터를 함께 빌드·쿠킹합니다. 설치 도구는 전체 쿠킹 빌드의 소스 지문과 APK 해시를 확인합니다. 이전 네이티브 교체 APK는 시작 크래시가 확인되어 `.apk.invalid`로 격리했으며 재사용하지 않습니다. 최신 Fab 적용 상태와 Mac 검증 결과는 [WeatherSystem.md](Docs/WeatherSystem.md)에 기록합니다.
