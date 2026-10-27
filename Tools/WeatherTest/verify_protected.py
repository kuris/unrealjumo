#!/usr/bin/env python3
"""Compare protected project files against hashes taken before weather work."""
import hashlib
import json
from pathlib import Path
import sys


def main():
    root = Path(__file__).resolve().parents[2]
    baseline = json.loads(Path(__file__).with_name("protected_baseline.json").read_text())
    changed = []
    for name, expected in baseline.items():
        path = root / name
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            changed.append(name)
    if changed:
        print("FAIL: protected files differ:\n" + "\n".join(changed))
        return 1
    print(f"PASS: {len(baseline)} protected files unchanged (levels, character, config, project).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
