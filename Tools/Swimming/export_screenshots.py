"""Collect game captures with their original pass/fail reports intact."""
from datetime import datetime
import json
from pathlib import Path
import shutil

root=Path(__file__).resolve().parents[2]
sources={
    'Mac':Path((root/'Saved/Swimming/latest_validation.txt').read_text().strip()),
    'Fold8_Unfolded':Path((root/'Saved/Swimming/latest_android.txt').read_text().strip()),
}
shore=root/'Saved/Swimming/latest_shore_controls.txt'
if shore.exists():sources['Fold8_ShoreControls']=Path(shore.read_text().strip())
reports={name:json.loads((path/'result.json').read_text()) for name,path in sources.items()}
out=root/'Screenshots/SwimmingSystem'/datetime.now().strftime('%Y%m%d-%H%M%S')
out.mkdir(parents=True)
lines=['# 선녀 수영 실제 실행 화면','','최종 수영 버튼과 자동 수영을 포함한 실행 검사 캡처입니다. 폴드는 펼친 화면만 검사했습니다.',
    '', '**전체 통과 여부는 각 result.json을 확인하세요.** Mac과 별도 해안 조작 복구 검사는 통과했습니다. 폴드 연속 자동 터치 검사에는 해안 복귀 시간 초과가 남아 있습니다. 사진만으로 전체 검증 성공을 뜻하지 않습니다.','']
for name,source in sources.items():
    target=out/name
    target.mkdir()
    shutil.copy2(source/'result.json',target/'result.json')
    lines.extend(['## '+name,'','원본 보고서 passed: '+str(reports[name]['passed']), '', '| 화면 | 파일 |','| --- | --- |'])
    for image in sorted(source.glob('*.png')):
        if image.stem=='failure':continue
        shutil.copy2(image,target/image.name)
        lines.append('| '+image.stem+' | ['+image.name+']('+name+'/'+image.name+') |')
    lines.append('')
(out/'README.md').write_text('\n'.join(lines))
(out/'manifest.json').write_text(json.dumps({name:str(path) for name,path in sources.items()},indent=2))
(out.parent/'latest.txt').write_text(str(out))
print(out)
