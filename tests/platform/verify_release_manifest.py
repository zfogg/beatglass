"""Verify a promoted release bundle and its hash manifest."""
import argparse
import hashlib
import json
from pathlib import Path


def verify(root: Path) -> list[str]:
    manifest_path = root / "release-manifest.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"invalid release manifest: {error}"]
    errors = []
    if manifest.get("release_manifest_schema") != 1:
        errors.append("unsupported release_manifest_schema")
    if manifest.get("decision") != "ACCEPTED":
        errors.append("release manifest decision is not ACCEPTED")
    for filename, field in (("readiness-report.json", "report_sha256"), ("review.json", "review_packet_sha256")):
        path = root / filename
        try:
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError as error:
            errors.append(f"missing promoted artifact {filename}: {error}")
            continue
        if actual != manifest.get(field):
            errors.append(f"release manifest hash mismatch: {filename}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("release_root", type=Path)
    args = parser.parse_args(); errors = verify(args.release_root)
    if errors:
        print("\n".join(errors)); return 1
    print("release manifest verified"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
