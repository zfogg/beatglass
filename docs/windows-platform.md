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
before restart. A separate bounded event queue carries the capture thread's
consecutive running and interrupted transitions for each start. The control owner
must map startup interruption to an error, apply the running event before allowing
PCM, and clear retained evidence on interruption. Events and PCM have no shared
delivery order; the integration owner must handle in-flight packets and event
invalidation. Device notifications, automatic
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
interruption and could be stopped and retried. With generated playback explicitly
enabled using `& "$env:TEMP/beatglass-windows-main/windows_endpoint_tests.exe" '[native-audio]' -s`,
the selected default Speakers (Realtek(R) Audio) endpoint captured a generated
440 Hz tone at 48 kHz stereo; the observed peak packet correlation amplitude was
0.119918. Deliberately withholding the consumer caused dropped frames and a
subsequent discontinuity at a frame-position gap. The test produces sound and is
excluded from ordinary CTest runs. A subsequent native run also stopped and
restarted that endpoint while playback remained active and verified that the
first new packet carried the fresh epoch 102 rather than the old epoch 101.
A prior version of the tone assertion failed
when phase changed between packets; the current assertion uses packet-local
correlation. These checks used base commit `6f927cb` plus the endpoint and draft
loopback changes. Endpoint disconnection, default-device changes, long-session
continuity, actual product UI and overlay remain unqualified. The backend polls
the selected endpoint's state approximately once per second and interrupts when
it is no longer active; this path has not been exercised by disconnecting a
device. The API documentation Docker build passed with Sphinx warnings treated as
errors. This draft does not satisfy the native audio/overlay qualification gate.

The control event stream has capacity for three events and emits at most two per
attempt, with one producer and one consumer. A 30-cycle test alternated immediate
stop with waiting for running; each stop joined the thread and discarded pending
events, and a new attempt started its event sequence at one. The test did not
disconnect hardware or test an interruption following a running event. This
private stream does not yet supply a shared `SourceAttempt`; the integration
adapter must bind an attempt identity, order delivery relative to epoch setup,
translate startup failures, and invalidate in-flight analysis. WASAPI device-frame
position jumps and resets, timestamp errors, and flagged discontinuities mark a
packet discontinuous. The epoch-relative frame count excludes unknown device gaps
and must not be treated as an uninterrupted physical timeline.

The device-position increment was built on Windows 11 Home 10.0.26300 x64
with GCC 15.2.0 using `cmake --build "$env:TEMP/beatglass-windows-main" -j 4`;
`ctest --test-dir "$env:TEMP/beatglass-windows-main" --output-on-failure`
passed 29/29. Unit tests exercise jumps, backwards resets, invalid timestamps,
and arithmetic overflow; the explicitly invoked `[native-audio]` test also
passed with generated playback. This does not simulate a physical device switch
or prove native device-position anomalies occur on this endpoint.
