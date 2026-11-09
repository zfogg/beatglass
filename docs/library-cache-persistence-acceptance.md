# Durable Library cache acceptance

Issue #1 requires cache reuse to survive reopening the library while remaining
strict about source and analysis identity. The SQLite acceptance case in
`tests/unit/library_sqlite_cache_test.cpp` exercises that boundary across two
cache instances:

- a completed result for the unchanged content hash, engine version,
  configuration, and mode is reused after reopening the database;
- changing the content identity produces a miss and publishes a new result;
- changing the analysis configuration also produces a miss and publishes a
  separate result.

The cache stores an opaque completed result, while callers remain responsible
for obtaining and revalidating the source identity before admission. This test
does not claim native decoder or filesystem qualification; those remain open
issue #1 gates.
