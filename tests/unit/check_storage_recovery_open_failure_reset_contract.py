from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
required = ('if (ok) refresh();', 'emit runsChanged({}, {});',
            'emit analysis(QStringLiteral("Analysis idle"));')
if any(marker not in source for marker in required):
    raise SystemExit("open failure must clear recovery presentation")
print("storage recovery open failure reset contract verified")
