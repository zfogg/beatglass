# Windows platform work

The first increment enumerates active render endpoints and identifies the current
multimedia default. It is not connected to the desktop and does not capture audio.
Discovery is a snapshot: a device can change while the list is being read.
Device notifications, selected-endpoint WASAPI loopback, restart and overlay
behavior remain to be implemented. WASAPI loopback captures the shared endpoint
mix; exclusive-mode, ASIO and other paths that bypass that mix are not guaranteed
to be captured. Playback is not rerouted.

On Windows 11 Home 10.0.26300 x64, base commit `6f927cb` plus this increment,
GCC 15.2.0 and Ninja, the following configured and built the core and Windows
endpoint tests outside the source tree, and CTest passed 24/24:

```powershell
cmake -S . -B "$env:TEMP/beatglass-windows-main" -G Ninja -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTING=ON "-DFETCHCONTENT_SOURCE_DIR_CATCH2=$env:TEMP/beatglass-windows-endpoints/_deps/catch2-src"
cmake --build "$env:TEMP/beatglass-windows-main" -j 6
ctest --test-dir "$env:TEMP/beatglass-windows-main" --output-on-failure
```

The Catch2 source directory above is the already fetched, hash-verified v3.8.1
source; omit the `FETCHCONTENT_SOURCE_DIR_CATCH2` argument for a fresh download.
The tests exercised native COM enumeration, but allow an empty endpoint list;
they do not establish the presence of a real device, capture, audio quality,
device switching, UI or overlay qualification. The README's Docker Sphinx
warnings-as-errors documentation build passed with output in `/tmp`.

Before integration, the shared source API needs typed enumeration/start errors,
ordered device and interruption notifications, process-owned epochs, and an
explicit validity and clock-domain representation for WASAPI timing. See
`phase1-contract-proposals.md`; these requirements are proposals, not changes to
the shared contract.
