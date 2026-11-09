"""Contract checks for live analysis queue progress accessibility."""

from pathlib import Path


ROOT = Path(__file__).parents[2]
QML = ROOT / "src" / "desktop" / "qml" / "Main.qml"
DOC = ROOT / "docs" / "issue-76-visual-implementation-audit.md"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    required = [
        'objectName: "analysisQueueProgress"',
        'Accessible.name: "Analysis queue progress"',
        'Accessible.value: visible ? text : ""',
        'Accessible.description: visible ? "Live worker progress and retry state: " + text',
        'Accessible.role: Accessible.Status',
        'window.analysisJobBridge.frames',
        'window.analysisJobBridge.canRetry',
        '"Analysis queue progress is unavailable."',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing batch progress accessibility markers: " + ", ".join(missing))
    if "adjacent Analysis queue progress surface now exposes" not in doc:
        raise AssertionError("missing batch progress accessibility documentation")
    print(f"batch progress accessibility contract: {len(required) + 1} checks passed")


if __name__ == "__main__":
    main()
