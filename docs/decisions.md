# Initial decisions and open questions

- Use C++20 and CMake; Catch2 v3.8.1 is fetched by URL and SHA-256 for tests. Network is needed at first configure. No production dependencies have yet been pinned or verified.
- The queue has one producer and one consumer, fixed-size slots and one reserved slot. Push copies PCM; pop copies to consumer-owned storage. A full/invalid/oversized block is dropped and the next accepted block is marked discontinuous; dropped complete frame count is observable. Invalid partial frames are not complete frames and cannot be counted precisely. Atomics must be lock-free. Epoch and sample position originate at the source.
- Positions are frames within an epoch; monotonic timestamps and publication times are nanoseconds. Mapping drift and capture clock semantics require a concrete backend contract. Snapshots are intended to be published as immutable copies, with bounded histories imposed by the future publisher. Scores are not probabilities.
- Do not average stereo blindly. Channel reduction and resampling remain undecided until measured and tested, including anti-phase material.
- libsndfile is a candidate decoder, not a committed selection. AAC/M4A, platform codec and licensing coverage remain unverified. aubio and Essentia licensing and buildability remain open; the local vcpkg catalog contains aubio but no Essentia port. Do not infer distribution compatibility from this observation.
- PipeWire, Core Audio taps, WASAPI loopback, and compositor/window behavior require upstream/API, version and native verification. No platform support is claimed.
