from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "const auto normalized = id.trimmed();" not in source or "values.push_back(normalized.toStdString())" not in source:
    raise SystemExit("batch discard IDs must be normalized before storage deletion")
print("storage recovery trim batch contract verified")
