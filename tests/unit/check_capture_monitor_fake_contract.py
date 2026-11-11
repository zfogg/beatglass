"""Keep the QML capture-monitor test double aligned with the production bridge."""
from pathlib import Path

source = Path(__file__).parents[1].joinpath("qml", "tst_navigation.qml").read_text(encoding="utf-8")
required = (
    'signal changed()',
    'onStatusChanged: changed()',
)
missing = [marker for marker in required if marker not in source]
if missing:
    raise SystemExit(f"capture monitor fake is missing: {', '.join(missing)}")
print("capture monitor fake contract verified")
