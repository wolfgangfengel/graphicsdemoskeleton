"""Verify an extracted reference archive without third-party dependencies."""
import hashlib
import json
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve() if len(sys.argv) == 2 else None
if root is None:
    raise SystemExit('Usage: python3 Support/VerifyReference.py EXTRACTED_REFERENCE_DIRECTORY')
manifest = json.loads((root / 'manifest.json').read_text(encoding='utf-8'))
failures = []
for entry in manifest['files']:
    path = (root / entry['path']).resolve()
    if root not in path.parents or not path.is_file():
        failures.append(entry['path'] + ': missing or invalid path')
        continue
    data = path.read_bytes()
    if len(data) != entry['bytes'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
        failures.append(entry['path'] + ': size/checksum mismatch')
if failures:
    raise SystemExit('\n'.join(failures))
print(f"Verified {len(manifest['files'])} files; source commit {manifest['source_commit']}")
