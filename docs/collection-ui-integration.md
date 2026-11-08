# Collection UI integration contract

The filesystem browser is a directory view, not proof that an indexed collection is available. A future collection table must consume immutable, bounded pages from a worker-owned query API. Each page needs a request generation, stable source identity, availability, metadata, analysis state and typed cancellation/error outcomes.

The GUI must clear a selected result when the folder, filter, sort order, model generation or cancellation changes. A late page or analysis result is displayed only when its request generation and source identity still match the current selection. Missing or moved files remain visible with a recoverable status; removing an entry never deletes the source audio.

Analysis pages are provisional until source identity, decoder/runtime configuration and the analyzed frame boundary are verified. Unknown key, chord or strength values remain unknown. Cache reuse requires exact source and dependency identity; otherwise the worker may return bounded uncached evidence only, clearly labeled as unverified.

All SQLite and decoder work stays off the GUI thread. Capture callbacks do not allocate epochs or publish UI data. A process-owned epoch allocator must be injected into live and offline consumers, and stale grants or results must be rejected after cancellation, restart or recreation.

The contract is a design boundary, not a claim that collection indexing or verified analysis is wired into the product.
