#!/usr/bin/env python3
"""Copy actual validation captures into a dedicated, indexed review folder."""
import argparse
from datetime import datetime
import json
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--mac', type=Path, required=True)
parser.add_argument('--android', type=Path, required=True)
args = parser.parse_args()
destination = root / 'Screenshots/DialogueSystem' / datetime.now().strftime('%Y%m%d-%H%M%S')
destination.mkdir(parents=True)
descriptions = {
    '00_exploration': '평상시 탐험 / 기존 UI',
    'approach': 'NPC 접근 / 말풍선·대화 버튼',
    'greeting': '대화 시작 / 첫 대사·선택지',
    'branch_1': '첫 번째 선택지 결과',
    'branch_2': '두 번째 선택지 결과',
    'sound': '선비 — 이상한 소리 분기',
    'tavern': '선비 — 주막 분기',
    'stranger': '포졸 — 낯선 사람 분기',
    'sighting': '포졸 — 목격담 분기',
    'closed': 'X로 종료 / 기존 UI 복구',
    'exit_choice': '종료 선택지 / 기존 UI 복구',
    'end_restored': '대화 종료 후 탐험·조작 복구',
    'end_controls_restored': '대화 종료 후 탐험·조작 복구',
    'post_dialogue_rain': '종료 후 날씨 버튼·Niagara 비',
    'post_dialogue_jump': '종료 후 점프 버튼·점프 동작',
    '01_patrol_seonbi': '선비 순찰',
    '02_patrol_pojol': '포졸 순찰',
    '03_jumo_idle': '주모 대기',
    '04_jumo_walk': '주모 걷기',
    '05_jumo_run': '주모 달리기',
    '06_jumo_jump': '주모 점프',
    '07_jumo_landed': '주모 착지',
    '08_rain_100': '강수량 100% / Niagara 비',
    '09_clear': '맑음 복귀',
    'face_player': '접근 시 NPC 정지·바라보기',
}
rows = ['# 대화 시스템 실제 실행 화면', '', 'Mac과 폴드8 펼친 화면의 실제 실행 캡처입니다. 커버 화면 검증은 이번 범위에서 제외했습니다.', '',
        '스크린샷은 화면 품질을 직접 확인하기 위한 자료입니다. 자동 동작 검증과 실제 터치 검증의 차이는 원본 result.json과 Docs/DialogueSystem.md를 참고하세요.', '']
manifest = []
for label, folder in [('Mac', args.mac), ('Fold8_Unfolded', args.android)]:
    report = json.loads((folder / 'result.json').read_text())
    target = destination / label
    target.mkdir()
    shutil.copy2(folder / 'result.json', target / 'result.json')
    captures = sorted(folder.glob('*.png'))
    if not captures:
        raise RuntimeError('No captures in ' + str(folder))
    rows += ['## ' + label, '', f'자동 보고서 통과 여부: {report.get("passed")}. 캡처 {len(captures)}장.', '', '| 파일 | 장면 |', '| --- | --- |']
    for path in captures:
        shutil.copy2(path, target / path.name)
        description = next((text for key, text in descriptions.items() if key in path.stem), '실행 화면')
        relative = label + '/' + path.name
        rows.append(f'| [{path.name}]({relative}) | {description} |')
        manifest.append({'file': relative, 'scene': description, 'source': str(path)})
    rows.append('')
(destination / 'README.md').write_text('\n'.join(rows) + '\n')
(destination / 'manifest.json').write_text(json.dumps(manifest, indent=2, ensure_ascii=False))
(root / 'Screenshots/DialogueSystem/latest.txt').write_text(str(destination))
print('DIALOGUE_SCREENSHOTS_EXPORTED:', destination, 'captures=', len(manifest))
