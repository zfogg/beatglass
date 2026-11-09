from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlaySettingsSummaryReviewAction"')
end = qml.index('objectName: "overlayPersistedRecoveryDiagnostic"', start)
block = qml[start:end]

assert 'text: window.overlayLaunchFallbackActive ? "Review recovery" : "Review overlay summary"' in block
assert "Overlay recovery reviewed" in block
assert "overlaySettingsSummary.forceActiveFocus()" in block
print("overlay recovery review action UI contract: ok")
