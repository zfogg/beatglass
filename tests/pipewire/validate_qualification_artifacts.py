from pathlib import Path
import sys
import json

REQUIRED = ("build.txt", "ctest.txt", "daemon-status.txt", "discovery.txt",
            "stream-format.txt", "callback-trace.txt", "lifecycle-trace.txt",
            "limitations.txt")

def main() -> int:
    if len(sys.argv) not in (2, 3) or (len(sys.argv) == 3 and sys.argv[2] != "--json"):
        print("usage: validate_qualification_artifacts.py ARTIFACT_DIR [--json]", file=sys.stderr)
        return 2
    root = Path(sys.argv[1])
    if not root.is_dir():
        print(f"artifact directory not found: {root}", file=sys.stderr)
        return 2
    missing = [name for name in REQUIRED if not (root / name).is_file()]
    linked = [name for name in REQUIRED if (root / name).is_symlink()]
    empty = [name for name in REQUIRED if (root / name).is_file() and not (root / name).read_text(encoding="utf-8").strip()]
    if missing:
        print("missing: " + ", ".join(missing), file=sys.stderr)
        return 1
    if linked:
        print("symlinked: " + ", ".join(linked), file=sys.stderr)
        return 1
    if empty:
        print("empty: " + ", ".join(empty), file=sys.stderr)
        return 1
    sizes = {name: (root / name).stat().st_size for name in REQUIRED}
    if len(sys.argv) == 3:
        print(json.dumps({"schema": "pipewire-qualification-v1", "verified": True, "artifact_dir": str(root), "files": sizes}, sort_keys=True))
    else:
        for name, size in sizes.items(): print(f"{name}: {size} bytes")
        print(f"PipeWire qualification artifacts verified ({len(REQUIRED)} files)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
