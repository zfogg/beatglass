"""Verify a promoted release bundle and its hash manifest."""
import argparse
import hashlib
import json
from pathlib import Path


def verify(root: Path, signoff_path: Path | None = None) -> list[str]:
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
    if "native_qualification_status" in manifest and manifest["native_qualification_status"] not in {"pending", "qualified"}:
        errors.append("invalid native qualification status")
    for filename, field in (("readiness-report.json", "report_sha256"), ("review.json", "review_packet_sha256")):
        path = root / filename
        try:
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError as error:
            errors.append(f"missing promoted artifact {filename}: {error}")
            continue
        if actual != manifest.get(field):
            errors.append(f"release manifest hash mismatch: {filename}")
    if signoff_path is not None:
        try:
            signoff = json.loads(signoff_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            errors.append(f"invalid release signoff: {error}")
        else:
            if signoff.get("release_signoff_schema") != 1:
                errors.append("unsupported release_signoff_schema")
            if signoff.get("decision") != "ACCEPTED":
                errors.append("release signoff decision is not ACCEPTED")
            if not isinstance(signoff.get("reviewer"), str) or not signoff["reviewer"].strip():
                errors.append("release signoff reviewer is required")
            if signoff.get("report_sha256") != manifest.get("report_sha256"):
                errors.append("release signoff report hash does not match manifest")
            if signoff.get("review_packet_sha256") != manifest.get("review_packet_sha256"):
                errors.append("release signoff packet hash does not match manifest")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("release_root", type=Path)
    parser.add_argument("--signoff", type=Path, help="require and verify a manual release signoff")
    args = parser.parse_args(); errors = verify(args.release_root, args.signoff)
    if errors:
        print("\n".join(errors)); return 1
    print("release manifest verified"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
