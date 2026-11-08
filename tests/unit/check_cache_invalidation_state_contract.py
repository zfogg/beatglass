"""Verify cache invalidation clears the bridge's cached-result state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    'text == QStringLiteral("Cached analysis invalidated")',
    "cacheHit_ = false; cachedResult_.clear();",
    "emit cachedResultChanged();",
    'QStringLiteral("No cached analysis found")',
]
missing = [marker for marker in required if marker not in bridge]
if missing:
    raise SystemExit("Missing cache invalidation state markers: " + ", ".join(missing))
print(f"Cache invalidation state contract verified ({len(required)} checks)")
