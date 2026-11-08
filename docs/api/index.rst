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

Repository documentation
------------------------

The Markdown documents below are included directly through MyST. They remain
Markdown source files for repository and GitHub use, while Sphinx provides one
searchable, cross-linked documentation site alongside the Hawkmoth C++ API
reference.

.. toctree::
   :maxdepth: 2
   :caption: Navigation

   ../README.md
   ../../README.md

.. toctree::
   :maxdepth: 1
   :caption: Status and coordination

   ../progress.md
   ../phase0-gate.md
   ../foundation-handoff.md
   ../platform-qualification-status.md

.. toctree::
   :maxdepth: 1
   :caption: Architecture and contracts

   ../contracts.md
   ../decisions.md
   ../phase1-contract-proposals.md
   ../audio-format-support.md
   ../collection-ui-integration.md
   ../pipewire-status-contract.md
   ../library-duration-contract.md
   ../dependency-identity-proposal.md
   ../epoch-handoff-proposal.md
   ../same-source-proposal.md
   ../library-catalog-contract.md
   ../library-file-details-contract.md
   ../library-fingerprint-contract.md
   ../library-format-support.md
   ../library-recovery-contract.md
   ../library-storage-open-contract.md
   ../library-storage-owner-contract.md
   ../visualization-playback-contract.md
   ../playback-visualization-frame-mapping.md

.. toctree::
   :maxdepth: 1
   :caption: Platform and qualification

   ../linux-qualification-plan.md
   ../linux-platform-increment.md
   ../linux-native-evidence-template.md
   ../macos-qualification.md
   ../macos-coreaudio-status.md
   ../macos-overlay.md
   ../windows-qualification.md
   ../windows-platform.md
   ../windows-overlay.md
   ../packaging-qualification.md
   ../native-qualification-evidence.md

.. toctree::
   :maxdepth: 1
   :caption: Design and experiments

   ../../design/v1/README.md
   ../../design/v1/PRODUCT.md
   ../../design/v1/DESIGN.md
   ../../design/v1/STATES.md
   ../../design/v1/REVIEW.md
   ../essentia-spike.md
   ../tonal-experiment.md
   ../issue-76-visual-implementation-audit.md

.. toctree::
   :maxdepth: 1
   :caption: Contributor guidance

   ../../AGENTS.md
   ../../skills/beatglass-documentation/SKILL.md
   ../../tests/standalone/README.md

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

.. cpp:autodoc:: include/beatglass/core/audio.hpp include/beatglass/core/audio_clock.hpp include/beatglass/core/channel_reduction.hpp include/beatglass/core/stream_continuity.hpp include/beatglass/core/audio_queue.hpp include/beatglass/capture/decoded_pcm_pump.hpp include/beatglass/capture/playback_output_adapter.hpp include/beatglass/capture/playback_pcm_queue.hpp include/beatglass/capture/detail/pipewire_discovery.hpp include/beatglass/capture/source.hpp include/beatglass/capture/coreaudio_internal.hpp include/beatglass/capture/coreaudio_fifo_internal.hpp include/beatglass/capture/coreaudio_tap_internal.hpp include/beatglass/capture/coreaudio_source_internal.hpp include/beatglass/capture/source_lifecycle.hpp include/beatglass/capture/byte_source.hpp include/beatglass/capture/file_reader.hpp

.. cpp:class:: beatglass::AudioQueue

   Bounded preallocated single-producer, single-consumer handoff. The concrete
   template parameters are documented in ``audio_queue.hpp``; the producer
   copies into preallocated storage and the consumer receives a complete block.

.. cpp:function:: bool beatglass::coreaudio::SourceBackend::stop() noexcept

   Attempt to quiesce capture. Return false and retain any callback context if
   the OS refuses to stop it. Call on the control owner, not the audio thread.

Library storage
---------------

Storage configuration is caller-owned. The desktop owner must choose and
validate a database file path before constructing worker-owned SQLite storage;
this interface does not select platform defaults or perform migration.

.. cpp:autodoc:: include/beatglass/library/storage_config.hpp include/beatglass/library/storage_owner.hpp include/beatglass/library/catalog.hpp include/beatglass/library/details.hpp

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

.. cpp:autodoc:: include/beatglass/core/results.hpp include/beatglass/core/estimator.hpp include/beatglass/core/strategies.hpp include/beatglass/core/presentation.hpp include/beatglass/core/presentation_gate.hpp include/beatglass/core/tempo_tracker.hpp include/beatglass/core/causal_replay.hpp include/beatglass/core/music_replay.hpp include/beatglass/core/file_analysis.hpp include/beatglass/core/tracking_machine.hpp include/beatglass/core/snapshot_publisher.hpp include/beatglass/core/playback_session.hpp include/beatglass/core/source_presentation_gate.hpp include/beatglass/core/visualization_presentation.hpp

Offline results preserve overlapping local tonal evidence separately from an
optional validated track-level key. Missing windows and scores remain unknown;
the value type does not itself provide database paging or persistence.

.. cpp:autodoc:: include/beatglass/core/offline_analysis.hpp include/beatglass/core/music_collector.hpp include/beatglass/core/visualization.hpp

Library jobs
------------

The provisional storage interface distinguishes a cache miss, cancellation,
stale input and storage error. Its worker owns the cancellation-versus-commit
decision; file verification accepts cooperative cancellation before that
decision. The interface and helper do not implement SQLite transactions.

.. cpp:autodoc:: include/beatglass/library/jobs.hpp include/beatglass/library/job_lifecycle.hpp include/beatglass/library/store.hpp include/beatglass/library/analysis_operation.hpp include/beatglass/library/analysis_runner.hpp include/beatglass/library/analysis_commit.hpp include/beatglass/library/detail/file_fingerprint.hpp include/beatglass/library/detail/job_executor.hpp include/beatglass/library/detail/sqlite_cache.hpp

Desktop application internals
-----------------------------

.. cpp:autodoc:: include/beatglass/desktop/macos_playback_session.hpp src/desktop/metadata.hpp src/desktop/live_presentation.hpp src/desktop/analysis_job_bridge.hpp src/desktop/analysis_presentation.hpp src/desktop/capture_monitor_bridge.hpp src/desktop/library_catalog_bridge.hpp src/desktop/platform/windows_endpoint_bridge.hpp src/desktop/platform/windows_endpoints.hpp src/desktop/platform/windows_loopback.hpp src/desktop/platform/windows_device_position.hpp src/desktop/platform/windows_loopback_session.hpp src/desktop/platform/windows_live_tempo.hpp src/desktop/platform/windows_live_music.hpp src/desktop/platform/windows_default_tempo.hpp src/desktop/platform/windows_dj_bridge.hpp src/desktop/platform/windows_overlay.hpp src/desktop/platform/windows_overlay_policy.hpp src/desktop/platform/macos_overlay.hpp src/desktop/platform/windows_playback_session.hpp src/desktop/platform/windows_waveout_devices.hpp src/desktop/platform/windows_waveout_playback.hpp src/desktop/playback_session_bridge.hpp src/desktop/storage_recovery_bridge.hpp src/desktop/theme_preferences.hpp src/desktop/visualization_presentation_bridge.hpp

.. cpp:function:: beatglass::windows::LoopbackSession::~LoopbackSession()

   Invalidate and stop the session on its control thread before destruction.
   The retained invalidation callback must not throw.

.. cpp:function:: beatglass::windows::LiveTempo::~LiveTempo()

   Stop capture and clear the estimate on the owning worker thread.

.. cpp:function:: beatglass::windows::DjBridge::~DjBridge()

   Stop and join the worker before destruction. Call on the GUI thread; an OS
   device operation that stalls can delay destruction.

.. cpp:function:: beatglass::windows::OverlayController::~OverlayController()

   Unlock the overlay and remove event filters. The caller retains ownership of
   both windows. Call on the GUI thread.

.. cpp:function:: beatglass::macos::OverlayController::~OverlayController()

   Unlock and hide the overlay and release the native adapter. The caller owns
   both windows. Call on the GUI thread.

.. cpp:function:: beatglass::macos::OverlayNative::~OverlayNative()

   Destroy the private native adapter on the GUI thread. It never owns the Qt
   window passed to its operations.

Experimental internals
----------------------

.. cpp:autodoc:: lib/core/tonal_tracker.hpp


.. cpp:autodoc:: lib/capture/file/virtual_io.hpp

.. cpp:autodoc:: lib/core/beat_tracker.hpp

CLI application internals
-------------------------

.. cpp:autodoc:: src/cli/state_label.hpp

