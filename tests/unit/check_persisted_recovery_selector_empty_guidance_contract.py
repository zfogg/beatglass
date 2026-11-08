"""Contract checks for persisted recovery selector empty-state guidance."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRuns"',
        'count > 0 ? "Choose one of " + count + " saved analysis runs to recover."',
        '"No saved analysis runs available."',
        'ToolTip.text: count > 0',
        'enabled: visible && count > 0',
        'Accessible.role: Accessible.ComboBox',
        'activeFocusOnTab: true',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery selector empty guidance markers: " + ", ".join(missing))
    print(f"persisted recovery selector empty guidance contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
