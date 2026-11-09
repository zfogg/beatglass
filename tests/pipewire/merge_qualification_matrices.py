"""Merge per-host qualification matrices into one handoff report."""
import json
import sys
from pathlib import Path

if len(sys.argv) < 3:
    raise SystemExit("usage: merge_qualification_matrices.py OUTPUT.json INPUT...json")
output, inputs = Path(sys.argv[1]), [Path(item) for item in sys.argv[2:]]
merged = None
for path in inputs:
    try:
        matrix = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError, UnicodeDecodeError):
        raise SystemExit(f"malformed qualification matrix: {path}")
    if matrix.get("schema") != "beatglass-native-qualification-matrix-v1":
        raise SystemExit(f"invalid qualification matrix schema: {path}")
    for platform_name, entry in matrix.get("platforms", {}).items():
        for evidence_path in entry.get("evidence", []):
            if (not isinstance(evidence_path, str) or evidence_path.startswith(("/", "\\")) or
                    ".." in Path(evidence_path).parts):
                raise SystemExit(f"unsafe evidence path for {platform_name}: {evidence_path}")
    if merged is None:
        merged = dict(matrix)
        merged["platforms"] = {}
    for name, entry in matrix["platforms"].items():
        if name in merged["platforms"] and merged["platforms"][name] != entry:
            raise SystemExit(f"conflicting qualification matrix platform: {name}")
        merged["platforms"][name] = entry
if merged is None or set(merged["platforms"]) != {"Linux", "Windows", "Darwin"}:
    raise SystemExit("merged qualification matrix platform set is incomplete")
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(merged, indent=2) + "\n", encoding="utf-8")
print(f"Merged {len(inputs)} qualification matrices into {output}")
