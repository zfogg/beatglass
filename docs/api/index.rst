Beatglass API reference
=======================

These interfaces describe the current foundation and are provisional. Headers
under ``include/beatglass`` are library interfaces, not a promise of an
installed or stable public API. ``src/desktop/metadata.hpp`` is application
private. Production headers under ``lib/`` are likewise private and must be
indexed here when introduced; documenting them does not install them or make
them public. Callers must respect each interface's ownership and threading rules.
The implementation and the issue's remaining product gates take precedence over
aspirational behavior; see the repository README and progress notes.

Audio and capture
-----------------

Source adapters report typed enumeration and start outcomes and deliver
causally ordered notifications to the control owner. A sequence gap or
delivery loss invalidates the active epoch. Clock anchors are optional and
carry a clock domain and optional uncertainty and latency; absence is not zero.
These interfaces require adapter integration and do not implement capture.
One process-owned ``SourceEpochAllocator`` may serve concurrent control and
worker requests; it must outlive those callers and is never called by an audio
callback. Issuance does not order worker completion or install stream formats.

.. cpp:autodoc:: include/beatglass/core/audio.hpp include/beatglass/core/audio_clock.hpp include/beatglass/core/channel_reduction.hpp include/beatglass/core/stream_continuity.hpp include/beatglass/core/audio_queue.hpp include/beatglass/capture/detail/pipewire_discovery.hpp include/beatglass/capture/source.hpp include/beatglass/capture/coreaudio_internal.hpp include/beatglass/capture/coreaudio_fifo_internal.hpp include/beatglass/capture/coreaudio_tap_internal.hpp include/beatglass/capture/coreaudio_source_internal.hpp include/beatglass/capture/source_lifecycle.hpp include/beatglass/capture/byte_source.hpp include/beatglass/capture/file_reader.hpp

.. cpp:function:: bool beatglass::coreaudio::SourceBackend::stop() noexcept

   Attempt to quiesce capture. Return false and retain any callback context if
   the OS refuses to stop it. Call on the control owner, not the audio thread.

.. cpp:function:: std::optional<std::int64_t> beatglass::coreaudio::SourceBackend::host_time_ns(std::uint64_t ticks) const noexcept

   Convert valid host ticks to backend monotonic nanoseconds, or return empty
   on failure or overflow. This is called on the control owner.

AudioSource stop
~~~~~~~~~~~~~~~~

.. cpp:function:: void beatglass::AudioSource::stop() noexcept

   Stop the source and quiesce all callbacks before returning. Call this on
   the owning control thread, not from a callback. The caller must stop the
   source before destruction. This manual entry preserves the declaration
   while working around Hawkmoth's handling of pure virtual ``noexcept``.

Analysis
--------

Snapshot timelines use an exclusive observed-frame boundary. Beats precede
that boundary, and chord segments are nonempty half-open intervals whose end
may equal it. Producers using the former inclusive boundary must adjust.

.. cpp:autodoc:: include/beatglass/core/results.hpp include/beatglass/core/estimator.hpp include/beatglass/core/strategies.hpp include/beatglass/core/presentation.hpp include/beatglass/core/presentation_gate.hpp include/beatglass/core/tempo_tracker.hpp include/beatglass/core/causal_replay.hpp include/beatglass/core/music_replay.hpp include/beatglass/core/file_analysis.hpp include/beatglass/core/tracking_machine.hpp include/beatglass/core/snapshot_publisher.hpp

Offline results preserve overlapping local tonal evidence separately from an
optional validated track-level key. Missing windows and scores remain unknown;
the value type does not itself provide database paging or persistence.

.. cpp:autodoc:: include/beatglass/core/offline_analysis.hpp

Library jobs
------------

The provisional storage interface distinguishes a cache miss, cancellation,
stale input and storage error. Its worker owns the cancellation-versus-commit
decision; file verification accepts cooperative cancellation before that
decision. The interface and helper do not implement SQLite transactions.

.. cpp:autodoc:: include/beatglass/library/jobs.hpp include/beatglass/library/job_lifecycle.hpp include/beatglass/library/store.hpp include/beatglass/library/detail/job_executor.hpp include/beatglass/library/detail/sqlite_cache.hpp

Desktop application internals
-----------------------------

.. cpp:autodoc:: src/desktop/metadata.hpp src/desktop/live_presentation.hpp src/desktop/platform/windows_endpoints.hpp src/desktop/platform/windows_overlay.hpp

.. cpp:function:: beatglass::windows::OverlayController::~OverlayController()

   Unlock the overlay and remove event filters. The caller retains ownership of
   both windows. Call on the GUI thread.

Experimental tonal worker internals
-----------------------------------

.. cpp:autodoc:: lib/core/tonal_tracker.hpp


Experimental beat-event worker
------------------------------

.. cpp:autodoc:: lib/core/beat_tracker.hpp

CLI application internals
-------------------------

.. cpp:autodoc:: src/cli/state_label.hpp
