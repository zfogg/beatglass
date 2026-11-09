"""Contract checks for persisted recovery failure/cancellation outcomes."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'target: typeof storageRecovery !== "undefined" ? storageRecovery : null',
        'function onAnalysisStatusChanged()',
        'storageRecovery.selectedRunId === "" || !storageRecovery.analysisRetryable',
        'storageRecovery.analysisStatus === "Analysis failed"',
        '"Persisted recovery run " + storageRecovery.selectedRunId + " failed; retry is available."',
        'storageRecovery.analysisStatus === "Analysis cancelled"',
        '"Persisted recovery run " + storageRecovery.selectedRunId + " was canceled; retry is available."',
        'retryPersistedRecovery.forceActiveFocus()',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery action outcome markers: " + ", ".join(missing))
    print(f"persisted recovery action outcomes contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
