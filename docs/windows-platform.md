# Windows platform work

The provisional backend enumerates active render endpoints and opens a selected
endpoint with WASAPI shared-mode loopback. It is not connected to the desktop.
Discovery is a snapshot: a device can change while the list is being read. A
dedicated thread owns COM and WASAPI objects and copies float packets into a
preallocated SPSC queue. Overflow drops complete chunks and marks the next
packet discontinuous. Packets carry caller-allocated epochs and QPC timestamps
in 100 ns units with explicit validity; frame positions count captured frames
within the epoch. Timestamp errors and WASAPI discontinuities require
reacquisition. Only 32-bit float mix formats are accepted. HRESULT failures are
observable; stop joins before discarding queued data. The caller must invalidate
presentation on interruption, discard retained packets and supply a new epoch
before restart. Status polling and the packet queue do not provide ordered control
notifications; integration must handle that race. Device notifications, automatic
restart, default-following and overlay behavior remain to be implemented. WASAPI loopback captures the shared endpoint
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

## Loopback development check

On the same Windows 11 machine and compiler, the loopback increment built with
`cmake --build "$env:TEMP/beatglass-windows-main" -j 6` and passed
`ctest --test-dir "$env:TEMP/beatglass-windows-main" --output-on-failure`
(26/26). An active render endpoint opened and stopped; an invalid ID reported
interruption and could be stopped and retried. No sound was played or inspected,
and queue overflow, disconnect, default-device changes, audio continuity, and
actual product UI remain unqualified. The API documentation Docker build passed
with Sphinx warnings treated as errors. This is a draft implementation and does
not satisfy the native audio/overlay qualification gate.
