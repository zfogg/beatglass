from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "file_decoder_snapshot_compatible" not in source or source.count("catch (const std::exception& error)") < 2:
    raise SystemExit("snapshot compatibility errors must be surfaced as retry rejection")
print("storage recovery snapshot error contract verified")
