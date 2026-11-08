"""Verify that platform qualification docs point at executable preparation tools."""
from pathlib import Path
import sys

REQUIRED = (
    "tests/windows/run_qualification.ps1",
    "tests/macos/run_qualification.sh",
    "tests/macos/verify_qualification_artifacts.sh",
    "docs/linux-qualification-plan.md",
    "docs/windows-qualification.md",
    "docs/macos-qualification.md",
)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    docs = (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
    readme = (root / "README.md").read_text(encoding="utf-8")
    required_disclaimers = ("does not establish native", "support language must remain experimental")
    required_platform_rows = ("| Windows |", "| macOS |", "| Linux |")
    required_matrix_columns = ("Current integrated/preparation evidence", "Native gates still required")
    missing = [path for path in REQUIRED if not (root / path).is_file()]
    unlinked = [path for path in REQUIRED if path not in docs and path not in ("docs/linux-qualification-plan.md", "docs/windows-qualification.md", "docs/macos-qualification.md")]
    if "docs/linux-qualification-plan.md" not in readme:
        unlinked.append("docs/linux-qualification-plan.md (README)")
    if "docs/platform-qualification-status.md" not in readme:
        unlinked.append("docs/platform-qualification-status.md (README)")
    unlinked.extend(f"status matrix row: {row}" for row in required_platform_rows if row not in docs)
    unlinked.extend(f"status matrix column: {column}" for column in required_matrix_columns if column not in docs)
    unlinked.extend(f"status disclaimer: {phrase}" for phrase in required_disclaimers if phrase not in docs)
    if missing or unlinked:
        print(f"missing tools: {missing}; undocumented tools: {unlinked}", file=sys.stderr)
        return 1
    print(f"qualification handoff covers {len(REQUIRED)} executable tools")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
