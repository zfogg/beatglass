from pathlib import Path

text = Path("src/desktop/qml/Main.qml").read_text()
for marker in ("discard requested for", "status will report if active work prevents removal", "discardPersistedRecoveryButton.forceActiveFocus()"):
    if marker not in text:
        raise SystemExit(f"missing active discard feedback marker: {marker}")
print("QML active-discard feedback contract verified")
