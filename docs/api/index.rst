Beatglass API reference
=======================

These interfaces describe the current foundation and are provisional. Headers
under ``include/beatglass`` are library interfaces, not a promise of an
installed or stable public API. ``src/desktop/metadata.hpp`` is application
private. Callers must respect each interface's ownership and threading rules.
The implementation and the issue's remaining product gates take precedence over
aspirational behavior; see the repository README and progress notes.

Audio and capture
-----------------

.. cpp:autodoc:: include/beatglass/core/audio.hpp include/beatglass/core/audio_clock.hpp include/beatglass/core/channel_reduction.hpp include/beatglass/core/stream_continuity.hpp include/beatglass/core/audio_queue.hpp include/beatglass/capture/detail/pipewire_discovery.hpp include/beatglass/capture/detail/pipewire_ingress.hpp include/beatglass/capture/detail/pipewire_monitor.hpp include/beatglass/capture/source.hpp include/beatglass/capture/source_lifecycle.hpp include/beatglass/capture/file_reader.hpp

AudioSource stop
~~~~~~~~~~~~~~~~

.. cpp:function:: void beatglass::AudioSource::stop() noexcept

   Stop the source and quiesce all callbacks before returning. Call this on
   the owning control thread, not from a callback. The caller must stop the
   source before destruction. This manual entry preserves the declaration
   while working around Hawkmoth's handling of pure virtual ``noexcept``.

Analysis
--------

.. cpp:autodoc:: include/beatglass/core/results.hpp include/beatglass/core/estimator.hpp include/beatglass/core/strategies.hpp include/beatglass/core/presentation.hpp include/beatglass/core/presentation_gate.hpp include/beatglass/core/tempo_tracker.hpp include/beatglass/core/file_analysis.hpp include/beatglass/core/tracking_machine.hpp include/beatglass/core/snapshot_publisher.hpp

Library jobs
------------

.. cpp:autodoc:: include/beatglass/library/jobs.hpp include/beatglass/library/job_lifecycle.hpp include/beatglass/library/store.hpp

Desktop application internals
-----------------------------

.. cpp:autodoc:: src/desktop/metadata.hpp
