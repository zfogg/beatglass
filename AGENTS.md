# Repository instructions

Follow GitHub issue #1 as the product specification and execution plan. Start with the foundation and its exit gate before delegating parallel feature work. Keep README, decisions, progress and platform support status accurate.

## Ownership and coordination

Keep application implementation and application-private headers under `src/`, reusable implementations under `lib/`, library headers under `include/beatglass/`, and all test code and fixtures under `tests/`. Keep build artifacts outside source directories. Use isolated branches or worktrees for parallel work, with explicit module ownership. Coordinate shared headers, schemas, central build configuration, dependencies and CI through the integration owner; propose contract changes before depending on them. Preserve user audio files and do not write tags without explicit user action.

## Audio and concurrency

The capture callback may only copy into preallocated bounded storage and record timing/status: no allocations, blocking locks, DSP, database, UI calls or ordinary logging. Define ownership, sample-position timing, epochs, overflow and discontinuities explicitly. Keep GUI, audio and database work on appropriate threads. Shut down and cancel safely, and reacquire after discontinuities. Product paths must use real analysis and actual output capture, not placeholders.

## Testing and qualification

Every implementing agent writes and runs meaningful unit tests alongside its work, runs relevant existing tests, and reports exact commands and results. Every bug fix requires a regression test or an appropriate integration/native scenario. Use CTest and pinned Catch2 v3 for shared C++ tests; keep CI passing on Linux, macOS and Windows. Record unrun checks honestly. Native qualification on actual Linux, Windows and macOS machines, including real desktop UI, audio and overlay scenarios, is required by issue #1; headless or CI results do not replace it. Record build identity, environment, evidence and limitations. Do not claim completion until all issue gates have been met.

## API documentation

Document every new or changed C++ header and its relevant declarations with Hawkmoth-compatible comments. Describe actual ownership, threading, timing, error behavior and provisional status where applicable; update the API reference and run the warnings-as-errors documentation build described in README.md. Coordinate shared-header comments and contracts with the integration owner.
