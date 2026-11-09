"""Verify the complete promoted release bundle before publication."""
import argparse
import importlib.util
import json
from pathlib import Path

spec = importlib.util.spec_from_file_location("manifest", Path(__file__).with_name("verify_release_manifest.py"))
manifest = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(manifest)


def verify(root: Path) -> list[str]:
    signoff = root / "signoff.json"
    if not signoff.is_file():
        return ["missing retained release signoff: signoff.json"]
    try:
        release_manifest = json.loads((root / "release-manifest.json").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return manifest.verify(root, signoff)
    errors = []
    if release_manifest.get("native_qualification_status") not in {"pending", "qualified"}:
        errors.append("release manifest lacks explicit native qualification status")
    return errors + manifest.verify(root, signoff)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("release_root", type=Path)
    args = parser.parse_args(); errors = verify(args.release_root)
    if errors:
        print("\n".join(errors)); return 1
    print("complete release bundle verified"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
