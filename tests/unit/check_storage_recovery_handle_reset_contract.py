from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
open_start = source.index("  void open(QString path) {")
close_start = source.index("  void close() {")
refresh_start = source.index("  void refresh() {")
if "handle_.reset();" not in source[open_start:close_start]:
    raise SystemExit("storage open must clear the previous analysis handle")
if "handle_.reset();" not in source[close_start:refresh_start]:
    raise SystemExit("storage close must clear the analysis handle")
print("storage transition handle reset contract verified")
