from pathlib import Path
import sys

REQUIRED = ("build.txt", "ctest.txt", "daemon-status.txt", "discovery.txt",
            "stream-format.txt", "callback-trace.txt", "lifecycle-trace.txt",
            "limitations.txt")

def main() -> int:
    if len(sys.argv) != 2:
        print("usage: validate_qualification_artifacts.py ARTIFACT_DIR", file=sys.stderr)
        return 2
    root = Path(sys.argv[1])
    missing = [name for name in REQUIRED if not (root / name).is_file()]
    if missing:
        print("missing: " + ", ".join(missing), file=sys.stderr)
        return 1
    print(f"PipeWire qualification artifacts verified ({len(REQUIRED)} files)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
