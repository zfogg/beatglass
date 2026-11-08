"""Verify that platform qualification docs point at executable preparation tools."""
from pathlib import Path
import sys

REQUIRED = (
    "tests/windows/run_qualification.ps1",
    "tests/macos/run_qualification.sh",
    "tests/macos/verify_qualification_artifacts.sh",
    "docs/linux-qualification-plan.md",
)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    docs = (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
    missing = [path for path in REQUIRED if not (root / path).is_file()]
    unlinked = [path for path in REQUIRED if path not in docs and path != "docs/linux-qualification-plan.md"]
    if "docs/linux-qualification-plan.md" not in (root / "README.md").read_text(encoding="utf-8"):
        unlinked.append("docs/linux-qualification-plan.md (README)")
    if missing or unlinked:
        print(f"missing tools: {missing}; undocumented tools: {unlinked}", file=sys.stderr)
        return 1
    print(f"qualification handoff covers {len(REQUIRED)} executable tools")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
