from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
required = ('coordinator_.close_storage();', 'emit runsChanged({}, {});',
            'emit analysis(QStringLiteral("Analysis idle"));')
missing = [marker for marker in required if marker not in source]
if missing:
    raise SystemExit("missing close reset markers: " + ", ".join(missing))
print("storage recovery close reset contract verified")
