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
    checklists = "\n".join((root / path).read_text(encoding="utf-8") for path in ("docs/windows-qualification.md", "docs/macos-qualification.md", "docs/linux-qualification-plan.md"))
    required_disclaimers = ("does not establish native", "support language must remain experimental")
    required_platform_rows = ("| Windows |", "| macOS |", "| Linux |")
    required_matrix_columns = ("Current integrated/preparation evidence", "Native gates still required")
    required_provenance_fields = ("source commit", "capture time", "environment/toolchain")
    required_output_display_artifacts = ("native-output-evidence", "native-display-evidence")
    required_capture_artifacts = ("native-capture-evidence", "native-recovery-evidence")
    required_bundle_artifacts = ("qualification-manifest", "qualification-artifacts", "native-playback-evidence", "native-overlay-evidence")
    required_matrix_scenarios = ("permission", "capture", "output-device", "recovery", "overlay", "fullscreen", "DPI", "monitor", "reduced-motion", "packaged")
    required_accessibility_scenarios = ("reduced-motion", "click-through", "keyboard")
    required_scenarios = ("permission", "device", "recovery")
    required_runtime_evidence = ("audio backend", "display server", "exact commands", "limitations")
    required_evidence_schema = ("timestamp", "logs", "screenshot")
    required_exit_gate = (
        "## Issue #1 exit gate",
        "all_platforms_ready: true",
        "aggregate_consistent: true",
        "actual desktop, audio, and overlay observations",
        "Do not claim issue #1 or v1 completion",
    )
    platform_rows = tuple(line for line in docs.splitlines() if line.startswith(("| Windows |", "| macOS |", "| Linux |")))
    required_sections = ("## Issue #76 UI gate", "Linux capture gate")
    missing = [path for path in REQUIRED if not (root / path).is_file()]
    unlinked = [path for path in REQUIRED if path not in docs and path not in ("docs/linux-qualification-plan.md", "docs/windows-qualification.md", "docs/macos-qualification.md")]
    if "docs/linux-qualification-plan.md" not in readme:
        unlinked.append("docs/linux-qualification-plan.md (README)")
    if "docs/platform-qualification-status.md" not in readme:
        unlinked.append("docs/platform-qualification-status.md (README)")
    unlinked.extend(f"status matrix row: {row}" for row in required_platform_rows if row not in docs)
    unlinked.extend(f"status matrix column: {column}" for column in required_matrix_columns if column not in docs)
    unlinked.extend(f"provenance linkage: {field}" for field in required_provenance_fields if field.lower() not in docs.lower())
    unlinked.extend(f"output/display artifact: {artifact}" for artifact in required_output_display_artifacts if artifact.lower() not in docs.lower())
    unlinked.extend(f"capture artifact: {artifact}" for artifact in required_capture_artifacts if artifact.lower() not in docs.lower())
    unlinked.extend(f"bundle artifact: {artifact}" for artifact in required_bundle_artifacts if artifact.lower() not in docs.lower())
    unlinked.extend(f"checklist artifact: {artifact}" for artifact in required_bundle_artifacts if artifact.lower() not in checklists.lower())
    unlinked.extend(f"matrix scenario: {scenario}" for scenario in required_matrix_scenarios if scenario.lower() not in docs.lower())
    unlinked.extend(f"accessibility scenario requirement: {scenario}" for scenario in required_accessibility_scenarios if scenario.lower() not in docs.lower())
    unlinked.extend(f"scenario requirement: {scenario}" for scenario in required_scenarios if scenario.lower() not in docs.lower())
    unlinked.extend(f"runtime evidence field: {field}" for field in required_runtime_evidence if field.lower() not in docs.lower())
    unlinked.extend(f"evidence schema field: {field}" for field in required_evidence_schema if field.lower() not in docs.lower())
    unlinked.extend(f"issue #1 exit-gate contract: {phrase}" for phrase in required_exit_gate if phrase.lower() not in docs.lower())
    for row in platform_rows:
        cells = [cell.strip() for cell in row.strip("|").split("|")]
        if len(cells) != 3 or not all(cells):
            unlinked.append(f"malformed status row: {row}")
    unlinked.extend(f"status section: {section}" for section in required_sections if section not in docs)
    unlinked.extend(f"status disclaimer: {phrase}" for phrase in required_disclaimers if phrase not in docs)
    if missing or unlinked:
        print(f"missing tools: {missing}; undocumented tools: {unlinked}", file=sys.stderr)
        return 1
    print(f"qualification handoff covers {len(REQUIRED)} executable tools")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
