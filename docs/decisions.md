# Initial decisions and open questions

- Use C++20 and CMake; Catch2 v3.8.1 is fetched by URL and SHA-256 for tests. Network is needed at first configure. A vcpkg registry baseline pins experimental production dependencies, but platform builds and distribution conditions are not yet fully verified.
- The queue has one producer and one consumer, fixed-size slots and one reserved slot. Push copies PCM; pop copies to consumer-owned storage. A full/invalid/oversized block is dropped and the next accepted block is marked discontinuous; dropped complete frame count is observable. Invalid partial frames are not complete frames and cannot be counted precisely. Atomics must be lock-free. Epoch and sample position originate at the source.
- Positions are frames within an epoch; monotonic timestamps and publication times are nanoseconds. Mapping drift and capture clock semantics require a concrete backend contract. An experimental publisher provides immutable snapshots with bounded item counts; integration with live analysis and the GUI remains open. Scores are not probabilities.
- Do not average stereo blindly. The experimental slice selects the loudest channel per block and tests anti-phase input; switching artifacts and resampling remain open.
- libsndfile is the decoder for the experimental file slice; the final distribution choice remains open. AAC/M4A, platform codec and licensing coverage remain unverified. aubio and Essentia licensing and buildability remain open; the local vcpkg catalog contains aubio but no Essentia port. Do not infer distribution compatibility from this observation.
- PipeWire, Core Audio taps, WASAPI loopback, and compositor/window behavior require upstream/API, version and native verification. No platform support is claimed.
- The visualization spectrum now uses KFR 6.3.1's DFT feature, replacing the in-house radix-2 transform. CMake fetches the pinned source with DFT enabled and runtime multi-architecture dispatch disabled; this avoids the MSVC heap exhaustion seen while building KFR's vcpkg multi-architecture library. The worker keeps the Hann window and single-sided amplitude normalization; KFR's real DFT uses caller-owned scratch storage for each transform. KFR is GPLv2+/commercial, so distribution licensing remains an open release decision.

## Experimental file slice

The slice uses libsndfile to decode interleaved float PCM at the original sample rate and aubio's causal tempo tracker with 1024-frame window and 512-frame hops. It selects the highest-energy channel per hop to avoid cancellation on anti-phase stereo. Switching channels can create artifacts; this is a provisional policy, not validated full-mix analysis. Partial final hops remain buffered and are not analyzed until complete. A positive aubio tempo estimate is presented without an invented confidence and expires after the provisional silence interval. No resampling, key, chord, streaming source, event timestamps, cancellation, or calibrated confidence is implemented.

Windows x64 MSVC builds used vcpkg aubio 2024-01-03#1 (upstream commit 152d6819b360c2e7b379ee3f373d444ab3df0895) and libsndfile 1.2.2#2, both core-only. Ubuntu 22.04 WSL used aubio 0.4.9 and distribution libsndfile/Qt 6.2.4. These are observations, not a reproducible cross-platform dependency lock. The Windows core-only libsndfile configuration does not establish MP3 or FLAC support. [libsndfile](https://libsndfile.github.io/libsndfile/) is LGPL 2.1 or 3; its [format matrix](https://libsndfile.github.io/libsndfile/formats.html) lists WAV/AIFF/FLAC, and its [FAQ](https://libsndfile.github.io/libsndfile/FAQ.html) says MP3 was added in 1.1.0, subject to build options. AAC/M4A is not established. aubio's bundled license is GPL-3.0-or-later. [Qt](https://doc.qt.io/qt-6/licensing.html) has LGPL/GPL/commercial options and module-specific conditions. [Essentia](https://essentia.upf.edu/licensing_information.html) offers AGPL/proprietary terms and separately licensed models; it is not integrated. The repository's current MIT license does not settle distribution obligations of linked dependencies; a distribution choice and full notice/third-party review remain open.

The experimental vcpkg manifest pins registry baseline `9e593bb18ea69cc5095e012465dcd675a822ed0d`, which selects aubio 2024-01-03#1, libsndfile 1.2.2#2, and optional Qt Declarative 6.11.1. Core-only features intentionally omit optional codecs. The manifest and presets still require validation across all triplets and desktop deployment; they are not a release distribution or a completed license review. Ambient system packages remain supported only for development experiments.

The experimental `TempoTracker` accepts arbitrary contiguous mono chunks on a worker thread, preserving partial hops and resetting on explicit discontinuity, epoch changes, or gaps. It provides an uncalibrated BPM and beat count only; source timing, beat timestamps, live replay and full estimator/snapshot integration remain open. The file path uses this same tracker.

## API reference

Use Hawkmoth with Sphinx to extract provisional C++ header comments, and publish the strict HTML build through GitHub Actions and Pages. The reference includes application-private headers without making them public API. The image and direct tool versions are pinned; Ubuntu transitive packages are not snapshot-pinned and remain a reproducibility limitation.

## Proposed prebuilt Qt SDK development path

Prefer the Qt **6.8.3 prebuilt desktop SDK** for foundation development, with vcpkg's pinned baseline retained for aubio and libsndfile. This proposal avoids compiling Qt on every fresh machine; it does not remove the existing experimental source-build preset. The new `prebuilt-qt-desktop` preset requires `BEATGLASS_QT_ROOT` to identify the matching SDK, requires Qt **exactly** 6.8.3, and does not enable the vcpkg `desktop` feature. Use a fresh build directory/core-only vcpkg install tree: an existing tree containing vcpkg Qt 6.11 mixed its `Qt6CoreTools` configuration with the 6.8 SDK and failed configuration. Never mix Qt versions in one install tree.

A Windows x64 MSVC 2022 SDK acquired from Qt's repository with `aqtinstall==3.3.0` was tested alongside the pinned core manifest. The Qt repository's metadata queried on 2026-11-07 listed `win64_msvc2022_64`, `linux_gcc_64`, and macOS `clang_64` for 6.8.3. Metadata availability is not a native build or architecture qualification. Qt's [6.8 supported-platform table](https://doc.qt.io/qt-6.8/supported-platforms.html) lists supported OS/compiler/architecture combinations but notes not all receive installer binaries. On Linux, check the SDK's glibc compatibility and required X11/Wayland libraries. Intel and Apple Silicon Mac SDK/runtime behavior, Linux desktop, Windows ARM and release deployment still require their own builds and tests.

Acquire an official Qt 6.8.3 desktop SDK for the target compiler through the [Qt installer](https://doc.qt.io/qt-6/get-and-install-qt-cli.html), or use the separately maintained `aqtinstall` tool as an experimental way to fetch Qt repository packages. The Windows smoke used `python -m pip install --target <temp>/beatglass-aqt aqtinstall==3.3.0` and `PYTHONPATH=<temp>/beatglass-aqt python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir <temp>/beatglass-qt --archives qtbase qtdeclarative qttools`. Pinning the SDK version and installer version does **not** yet pin every downloaded archive or Python transitive dependency: retain trusted archive checksums or an immutable internal artifact and a fully hashed installer dependency lock before claiming bit-for-bit reproducibility. Qt's online repository availability and mirror behavior also remain external dependencies.

For deployment, Qt's platform documentation describes [Windows windeployqt](https://doc.qt.io/qt-6.8/windows-deployment.html), [macOS deployment](https://doc.qt.io/qt-6.8/macos-deployment.html), and [Linux deployment](https://doc.qt.io/qt-6.8/linux-deployment.html); each actual package must be built and tested on its target platform. The Windows SDK includes `windeployqt`, which ran during this build. Replacing source-built Qt with binaries does not remove license duties: review the exact Qt modules and bundled third-party notices, applicable LGPL/GPL or commercial terms, corresponding-source and relinking/replaceability requirements, and distribution of runtime dependencies. Qt's [licensing documentation](https://doc.qt.io/qt-6.8/licensing.html) notes module-specific licensing. aubio, libsndfile, codecs and any future Essentia integration still need their own compatibility and distribution review. This proposal is not a release-license clearance.
## Proposed v1 decoder feature set

The vcpkg manifest now requests libsndfile `external-libs` and `mpeg` so the selected build can decode FLAC and MP3 as well as PCM WAV and AIFF. The port maps those features to `ENABLE_EXTERNAL_LIBS` and `ENABLE_MPEG`; upstream documents the [build options](https://github.com/libsndfile/libsndfile) and identifies [mpg123 for MPEG decoding and LAME for encoding](https://github.com/libsndfile/libsndfile/blob/master/CHANGELOG.md). The current vcpkg feature includes the LAME encoder even though Beatglass does not encode or modify user files. Generated fixtures exercise decoding and byte preservation, not format completeness or musical accuracy. AAC/M4A remains outside this decoder selection; the upstream [feature request](https://github.com/libsndfile/libsndfile/issues/389) was open on 2026-11-07.

At the pinned registry baseline the port metadata identifies libsndfile as LGPL-2.1-or-later; libFLAC, libogg, libvorbis and Opus as BSD-3-Clause; mpg123 as LGPL-2.1-or-later; and mp3lame as LGPL-2.0-only. These are package declarations to audit against actual source notices, build flags and transitive dependencies, not a complete legal determination. aubio is GPL-3.0-or-later; Essentia's AGPL/proprietary selection remains open. The repository's MIT source license alone is not a complete binary-distribution plan. Do not publish a combined build until license compatibility, notices, source/relink obligations and any required codec terms have been reviewed for the chosen artifacts.

For the current aubio-based slice, the proposed open-source distribution path is to convey the combined application under GPL-3.0-or-later while retaining the repository's MIT grant for its own code, supplying corresponding source, build scripts and required notices. GNU's [license guidance](https://savannah.gnu.org/maintenance/GnuGplTwoOrLater/) discusses conversion of LGPLv2-family code to GPL terms, but the exact source notices, link modes and obligations of the selected binary dependencies still require review. This is a candidate path, not authorization to publish an artifact. Reassess the whole distribution if Essentia or its separately licensed models are added.
## Platform API groundwork (not product support)

- [Qt 6.11 supported platforms](https://doc.qt.io/qt-6.11/supported-platforms.html) lists Windows 10 1809+ x86-64 with MSVC 2022, Windows on ARM64 with MSVC 2022, and macOS 13+ x86-64/arm64 with Xcode 15 or newer. Linux configurations are distribution-specific; its table includes Ubuntu 22.04 x86-64 and 24.04 x86-64/arm64. These are Qt's configurations, not Beatglass minimum-version guarantees; the actual pinned port and native capture backends still require verification.
- [Apple's Core Audio tap sample](https://developer.apple.com/documentation/coreaudio/capturing-system-audio-with-core-audio-taps) requires macOS 14.2 or later for the sample, an `NSAudioCaptureUsageDescription`, and user permission. A future Beatglass tap adapter must verify its own minimum OS and permissions and must not mute or reroute playback.
- [miniaudio's loopback example](https://github.com/mackron/miniaudio/blob/master/examples/simple_loopback.c) identifies WASAPI as its built-in loopback backend. This is not evidence of a working Beatglass endpoint selector, permissions, recovery, or non-Windows capture.

All supported CPU/OS combinations, packaging, overlay/compositor behavior, devices, and runtime audio paths remain unqualified.

## Capture and overlay feasibility notes

- [Microsoft's WASAPI documentation](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording) describes loopback on a render endpoint in shared mode; exclusive-mode loopback is unavailable and protected streams can be excluded. The adapter must expose the selected endpoint and recovery errors; this research does not establish working capture.
- [PipeWire's loopback module documentation](https://docs.pipewire.org/page_module_loopback.html) documents selecting a target node and `stream.capture.sink` for sink monitor ports. The future capture adapter still needs native device enumeration, routing, permission and recovery tests; no backend exists yet.
- [Qt window flags](https://doc.qt.io/qt-6/qt.html) express stay-on-top and input transparency as window-system hints. The [wlr layer-shell protocol](https://wayland.app/protocols/wlr-layer-shell-unstable-v1) describes top and overlay layers where a compositor exposes it, including compositor-dependent placement and ordering. Neither is proof of universal fullscreen overlays: probe compositor capability, preserve a normal/compact fallback, and test X11, Wayland compositors, macOS Spaces and Windows behavior natively.

For Windows x64, Qt's official repository reported the following SHA-256 values on 2026-11-07 for the three 6.8.3 MSVC 2022 base archives listed by `aqt --dry-run` (prefix `6.8.3-0-202503201308`, suffix `-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z`):

- `qtbase`: `41688269fac0565db956c66d9eecae777d16197e0c02cd81b88640c1f5d73d3f`
- `qtdeclarative`: `c2f9f481bf6e8fb004530f20b820fbc77d4ee6dcca24d1ea1d1863d99c5dabf1`
- `qttools`: `76ac7b8c86ca72b6608f1d41ee848aeab19663536990b4968ce52db8ec980bc3`

They were read from the respective `.sha256` files in the [official Qt repository](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/qt.qt6.683.win64_msvc2022_64/). They are upstream-published checksums, not independently retained or rehashed original archives from the prior smoke run. Future automated acquisition must verify fixed hashes against downloaded archives and lock the acquisition tool's transitive dependencies; other platforms need their own artifact manifests.
# Phase 1 source contract proposal

The proposed source API distinguishes enumeration cancellation/errors and
typed start failures. Control notifications carry attempt and serial sequence;
gaps and delivery loss fail closed. The process coordinator owns epoch
allocation, while optional anchors expose clock domain and known uncertainty
and latency. Existing adapters must migrate; no platform backend is provided.

# Process epoch allocation

PR #63 made a single coordinator-owned allocator safe to issue epochs concurrently to live and
offline control/workers. It is not an audio-callback API and issuance does not
replace per-source transition ordering or cross-session identity/draining.
Application ownership and injection into each active path remain integration work.

# Phase 1 cache contract proposal

Lookup and publication report typed outcomes. The database worker decides
cancellation before starting commit; after that point it resolves commit or
error and must recover indeterminate outcomes by run ID. This is an interface
proposal, not a claim that database transactions or filesystem identity races
have been implemented or qualified.

# Phase 1 snapshot boundary

Snapshot publication uses exclusive `observed_through` frame boundaries and
nonempty half-open chord intervals. A beat at the boundary and an empty chord
are now rejected; existing producers must adjust. This does not qualify any
native capture or presentation path.

# Offline result contract

PR #57 defined owned offline results separately from live snapshots. Preserve overlapping
local tonal evidence and optional beat strengths; rolling keys are not
track-level keys. Storage, paging, cache identity and result serialization
remain subject to integration review and are not implemented by this value type.

## Composite analysis identity (provisional)

A build manifest may record pinned dependency digests and the complete effective analysis configuration, but embedded build metadata alone is not proof of dynamically loaded code. Cache eligibility must be granted only after the actual executable and linked dependency artifacts have been verified under a platform-specific, race-resistant policy, including relevant transitive DSP dependencies. Unknown provenance disables composite cache lookup and publication. The worker may still return bounded in-memory evidence after successful source verification; it must not fill identity fields with invented values. This remains a proposal, not a claim of a verified packaging mechanism.

The provisional [dependency identity proposal](dependency-identity-proposal.md) distinguishes platform loader and code-signature evidence from exact mapped-artifact verification, covers transitive dependencies and effective configuration, and specifies fail-closed tests. It does not enable cache reuse.
# Analysis job UI contract (issue #76)

The desktop analysis detail panel intentionally reports `Queued`, `Running`,
and `Analysis progress unavailable` from `AnalysisJobBridge.frames`. The
`Retry analysis` control remains disabled until the bridge exposes a real
operation handle with cancellation, retry, and source identity. A future
implementation must preserve the selected source URL, reject stale results
after replacement, and publish an explicit terminal error before enabling
retry. The current UI must not imply that frame counts are source-attributed;
it labels them provisional until a source-linked result is published.
