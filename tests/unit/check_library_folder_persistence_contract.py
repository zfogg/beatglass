from pathlib import Path

qml = (Path(__file__).parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (Path(__file__).parents[2] / "src/desktop/library_preferences.hpp").read_text(encoding="utf-8")
for marker in ("libraryPreferences.folder", "libraryPreferences.folder = url.toString()", "Q_PROPERTY(QString folder"):
    if marker not in qml + bridge:
        raise SystemExit(f"Missing Library folder persistence marker: {marker}")
print("library folder persistence contract verified")
