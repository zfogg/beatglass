#!/usr/bin/env python3
"""Guard the worker-owned StorageOwner lifetime contract.

This is intentionally source-level: the contract is about thread ownership and
the close precondition, which cannot be exercised safely by the lightweight
unit-test harness.
"""
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "include" / "beatglass" / "library" / "storage_owner.hpp"


def main() -> None:
    text = HEADER.read_text(encoding="utf-8")
    required = (
        "Worker-owned SQLite storage lifetime",
        "Close storage on the owning worker",
        "Callers must stop/cancel and join work",
        "does not wait\n   * for executor jobs",
        "invalidates the cache pointer",
    )
    missing = [phrase for phrase in required if phrase not in text]
    if missing:
        raise AssertionError(f"StorageOwner lifetime contract missing: {missing}")
    if "void close() noexcept;" not in text:
        raise AssertionError("StorageOwner::close must remain noexcept")


if __name__ == "__main__":
    main()
