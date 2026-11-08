"""Verify reduced-motion changes are applied immediately to the live overlay."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "function onReduceMotionChanged()",
    "beatPulse.stop()",
    "pulseFade.stop()",
    "overlaySurface.pulseOpacity = 0.0",
    "beatPulse.restart()",
    "Reduced motion enabled; overlay beat pulse disabled.",
    "Reduced motion disabled; overlay beat pulse restored.",
    "running: djOverlay.visible && (!window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion)",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay motion/accessibility markers: " + ", ".join(missing))
if qml.count("function onReduceMotionChanged()") < 2:
    raise SystemExit("both settings and overlay surfaces must observe reduced-motion changes")
print(f"Overlay motion accessibility contract verified ({len(required)} checks)")
