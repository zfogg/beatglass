from pathlib import Path

source = Path("include/beatglass/library/storage_owner.hpp").read_text()
required = (
    "after submitted jobs are quiescent",
    "does not wait",
    "invalidates the cache pointer",
)
for phrase in required:
    if phrase not in source:
        raise SystemExit(f"missing close ownership contract: {phrase}")
print("storage owner shutdown ownership contract verified")
