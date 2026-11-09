"""Convert native script output into a conservative portable evidence manifest."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from datetime import datetime

SCENARIOS = ("capture", "recovery", "output", "display", "playback", "overlay")


def ingest(root: Path, output: Path) -> list[str]:
    errors = []
    identity = {}
    identity_path = root / "qualification-manifest.txt"
    if identity_path.is_file():
        for line in identity_path.read_text(encoding="utf-8").splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                if key in identity:
                    errors.append(f"duplicate identity key: {key}")
                identity[key] = value
    else:
        errors.append("missing qualification-manifest.txt")
    if not re.fullmatch(r"[0-9a-fA-F]{40}", identity.get("commit", "")):
        errors.append("invalid source commit identity")
    if not identity.get("captured_at", "").strip():
        errors.append("missing capture time")
    else:
        try:
            datetime.fromisoformat(identity["captured_at"].replace("Z", "+00:00"))
        except ValueError:
            errors.append("invalid capture timestamp")
    artifacts = []
    hashes_path = root / "qualification-artifacts.sha256"
    if not hashes_path.is_file():
        errors.append("missing qualification-artifacts.sha256")
    else:
        seen_paths = set()
        for line in hashes_path.read_text(encoding="utf-8").splitlines():
            parts = line.split(None, 1)
            if len(parts) != 2:
                errors.append(f"malformed artifact hash line: {line}"); continue
            digest, relative = parts
            path = (root / relative).resolve()
            if root.resolve() not in path.parents or not path.is_file():
                errors.append(f"missing or escaping artifact: {relative}"); continue
            normalized = Path(relative).as_posix()
            if normalized in seen_paths:
                errors.append(f"duplicate artifact path: {normalized}"); continue
            seen_paths.add(normalized)
            if path.stat().st_size == 0:
                errors.append(f"empty qualification artifact: {normalized}"); continue
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
            if actual != digest.lower():
                errors.append(f"artifact hash mismatch: {relative}")
            artifacts.append({"path": normalized, "sha256": actual, "role": "qualification-artifact"})
    referenced = {
        key: identity[key] for key in ("playback_evidence", "overlay_evidence", "display_topology", "audio_inventory")
        if identity.get(key)
    }
    known_paths = {item["path"] for item in artifacts}
    for key, relative in referenced.items():
        path = (root / relative).resolve()
        normalized = Path(relative).as_posix()
        if root.resolve() not in path.parents or not path.is_file():
            errors.append(f"missing or escaping referenced evidence: {relative}"); continue
        if normalized in known_paths:
            continue
        artifacts.append({"path": normalized, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "role": key})
    artifacts.sort(key=lambda item: (item["path"], item["role"]))
    report = {"platform": identity.get("platform", "unknown"), "commit": identity.get("commit", ""),
              "captured_at": identity.get("captured_at", ""), "environment": identity.get("environment", "native script output"),
              "artifacts": artifacts, "scenarios": [{"id": scenario, "status": "PENDING"} for scenario in SCENARIOS]}
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path); parser.add_argument("output", type=Path)
    args = parser.parse_args(); errors = ingest(args.root, args.output)
    if errors: print("\n".join(errors)); return 1
    print(f"wrote conservative evidence manifest: {args.output}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
