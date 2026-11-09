from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
for marker in ('AnalysisOperation operation;', 'make_file_decode_operation(found->source_path', 'Retry rejected: ') :
    if marker not in source:
        raise SystemExit(f"missing retry operation error marker: {marker}")
print("storage recovery operation error contract verified")
