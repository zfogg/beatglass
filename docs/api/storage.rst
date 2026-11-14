Library storage
===============

Storage configuration is caller-owned. The desktop owner must choose and validate a database path before constructing worker-owned SQLite storage.

.. cpp:autodoc:: include/beatglass/library/storage_config.hpp include/beatglass/library/storage_owner.hpp include/beatglass/library/catalog.hpp include/beatglass/library/details.hpp include/beatglass/library/source_detail.hpp

.. cpp:function:: std::optional<std::int64_t> beatglass::coreaudio::SourceBackend::host_time_ns(std::uint64_t ticks) const noexcept

   Convert valid host ticks to backend monotonic nanoseconds, or return empty on failure or overflow.

.. cpp:function:: void beatglass::AudioSource::stop() noexcept

   Stop the source and quiesce all callbacks before returning. Call this on the owning control thread.
