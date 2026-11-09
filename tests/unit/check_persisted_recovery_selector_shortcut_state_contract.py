"""Contract checks for persisted recovery selector shortcut state."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryShortcut"',
        'sequence: "Ctrl+Shift+R"',
        'enabled: typeof persistedRecoveryRuns !== "undefined" && persistedRecoveryRuns.enabled',
        'persistedRecoveryRuns.forceActiveFocus()',
        '"Persisted recovery selector focused; choose one of " + persistedRecoveryRuns.count + " saved runs to retry or discard."',
        'context: Qt.ApplicationShortcut',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery selector shortcut state markers: " + ", ".join(missing))
    print(f"persisted recovery selector shortcut state contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
