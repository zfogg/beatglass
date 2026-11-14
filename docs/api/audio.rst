Audio and capture
=================

Source adapters report typed enumeration and start outcomes and deliver causally ordered notifications to the control owner. A sequence gap or delivery loss invalidates the active epoch. Clock anchors are optional; absence is not zero. These interfaces require adapter integration and do not implement capture.

.. cpp:autodoc:: include/beatglass/core/audio.hpp include/beatglass/core/audio_clock.hpp include/beatglass/core/channel_reduction.hpp include/beatglass/core/stream_continuity.hpp include/beatglass/core/audio_queue.hpp include/beatglass/capture/decoded_pcm_pump.hpp include/beatglass/capture/playback_output_adapter.hpp include/beatglass/capture/playback_pcm_queue.hpp include/beatglass/capture/detail/pipewire_discovery.hpp include/beatglass/capture/detail/pipewire_native_callbacks.hpp include/beatglass/capture/detail/pipewire_stream_owner.hpp include/beatglass/capture/pipewire_source.hpp include/beatglass/capture/source.hpp include/beatglass/capture/source_selection.hpp include/beatglass/capture/coreaudio_internal.hpp include/beatglass/capture/coreaudio_fifo_internal.hpp include/beatglass/capture/coreaudio_tap_internal.hpp include/beatglass/capture/coreaudio_source_internal.hpp include/beatglass/capture/source_lifecycle.hpp include/beatglass/capture/byte_source.hpp include/beatglass/capture/file_reader.hpp

.. cpp:class:: beatglass::AudioQueue

   Bounded preallocated single-producer, single-consumer handoff.

.. cpp:function:: bool beatglass::coreaudio::SourceBackend::stop() noexcept

   Attempt to quiesce capture. Call on the control owner, not the audio thread.
