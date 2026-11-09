from pathlib import Path

source = Path("lib/library/storage_owner.cpp").read_text()
expected = 'catch (const std::invalid_argument&) { return std::nullopt; }'
if expected not in source:
    raise SystemExit("storage owner retry lookup must treat malformed IDs as absent")
print("storage owner retry lookup contract verified")
