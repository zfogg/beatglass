from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "discardAnalysis"',
    'objectName: "discardAnalysisBatch"',
    'id: discardAnalysisDialog',
    'id: discardAnalysisBatchDialog',
    'standardButtons: Dialog.Ok | Dialog.Cancel',
    'window.analysisJobBridge.discard()',
    'window.analysisJobBridge.discardBatch()',
    'Confirmation is required.',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing QML discard recovery markers: " + ", ".join(missing))
for name in ("discardAnalysis", "discardAnalysisBatch"):
    if qml.count(f'objectName: "{name}"') != 1:
        raise SystemExit(f"Expected exactly one {name} control")
print(f"QML discard recovery contract verified ({len(required)} checks)")
