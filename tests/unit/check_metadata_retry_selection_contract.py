from pathlib import Path

qml = (Path(__file__).parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
if qml.count("metadataRetryPending = false") < 2:
    raise SystemExit("Metadata retry cooldown must reset on source selection and clear")
print("metadata retry selection contract verified")
