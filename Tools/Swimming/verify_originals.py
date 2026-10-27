"""Check the original fairy, source GLB and weather map against the swim baseline."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
root=Path(__file__).resolve().parents[2]
manifests=sorted((root/'Saved/Swimming/Backups').glob('*/original_hashes.json'))
if not manifests: raise RuntimeError('No pre-swimming original hashes recorded')
entries=json.loads(manifests[0].read_text())
changed=[name for name,digest in entries.items()
    if not (root/name).is_file() or hashlib.sha256((root/name).read_bytes()).hexdigest()!=digest]
if changed: raise RuntimeError('Original assets changed: '+str(changed))
subprocess.run([sys.executable,str(root/'Tools/WeatherTest/verify_protected.py')],check=True)
print('SWIMMING_ORIGINALS_PASS:',len(entries),'files unchanged')
