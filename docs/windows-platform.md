# Windows platform work

The provisional backend enumerates active render endpoints and opens a selected
endpoint with WASAPI shared-mode loopback. This integration branch connects it
to the Windows desktop DJ page for experimental default-output tempo only.
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
invalidation. A worker-owned follower polls for the multimedia default and
retries after source failure; native notification callbacks and full desktop
integration remain outstanding. WASAPI loopback captures the shared endpoint
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

The merged shared source contract defines typed outcomes, ordered transitions,
process-owned epochs and optional clock anchors. The Windows private adapter
uses its lifecycle and epoch allocator; typed discovery/start outcomes and clock
anchor conversion remain integration work.

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
adapter binds a control-thread attempt and a process-owned epoch, applies ordered
events before accepting PCM, translates startup interruption to an error, and
invokes the owner's required invalidation callback on failure, interruption,
newly observed queue loss, discontinuous or untimed PCM, and stop. The callback
must clear publication and reject in-flight work; the adapter does not revoke a
packet already returned to an analysis worker. The control owner must poll and
deliver invalidation promptly, install format before analysis, and ensure the
callback and allocator outlive the session. WASAPI's device
position is currently not used to detect extra gaps: flagged discontinuities and
timestamp errors are marked, but the epoch-relative frame count excludes unknown
device gaps and must not be treated as an uninterrupted physical timeline.

Against reviewed source-contract PR #28 at `5b81fcc`, Windows 11 x64 GCC 15.2.0
`cmake --build "$env:TEMP/beatglass-windows-main" -j 4` and
`ctest --test-dir "$env:TEMP/beatglass-windows-main" --output-on-failure`
passed 34/34. Tests covered synchronous and asynchronous start failure, mandatory
invalidation, process epoch retention, running transition and repeated stop/start.
Physical device interruption, stale-result rejection by a real analysis worker,
and the desktop integration remain untested or absent.

An experimental worker-only path now feeds selected-endpoint packets through
`reduce_loudest_channel` and the existing aubio `TempoTracker`. It publishes only
an optional BPM to its caller and clears it on stop, source errors, queue loss,
discontinuity, malformed PCM, one second of silence or a one-second packet stall.
It must be pumped regularly by its owning worker and does not provide beat phase,
key or chords. The one-second silence
threshold is provisional.

On Windows 11 Home 10.0.26300 x64, MSVC 19.44 and the existing pinned codec tree,
`cmake --build "$env:TEMP/beatglass-wasapi-slice" --config Debug --target windows_endpoint_tests -j 4`
followed by the opt-in `windows_endpoint_tests.exe '[native-tempo]'` test played
a generated 120 BPM click track through Speakers (Realtek(R) Audio), captured it
via loopback and obtained 120.724 BPM in one run. The test stopped playback and
observed the estimate clear. The `[native-switch]` test opened and stopped
Realtek Digital Output and Speakers in turn with fresh epochs and invalidation;
it did not change the OS default or physically disconnect either endpoint.
The regular CTest selection `-R 'Live tempo|Session|endpoint|capture startup'`
passed 8/8. The first MSVC build exposed a Windows `max` macro conflict, corrected
with `NOMINMAX`, and the rebuilt target passed. These synthetic and endpoint tests
do not establish music accuracy, real physical
interruption recovery, or complete native DJ and overlay qualification.

Hosted Windows CI later reported three device-dependent tests as failures even
though Catch2 skipped them because the runner had no active render endpoint.
The Windows Catch discovery properties accidentally split a semicolon-separated
label into malformed property pairs, preventing CTest from recognizing the skip
exit code. The registration now uses a single label. A dedicated skip-propagation
regression was recognized as skipped locally; the targeted CTest selection passed
11 cases with that one deliberate skip. Hosted rerun results are pending.

After an explicit start, the private default follower polls active render endpoints
approximately once per second on the worker thread. Stop disables future polling
until a new start. A changed or missing default clears evidence
before stopping the old source; failures are retried after a delay with fresh
epochs. It exposes waiting/starting/running/error status and coalesces
strictly-revisioned `PresentationUpdate` snapshots and clears. The UI owner must
transfer updates to the GUI thread, preserve clears and ordering, consume status,
and call stop and deliver its clear before destruction. Revision exhaustion or a
fatal error requires a separate session boundary to clear the display. On this
integration branch, the Windows DJ page connects the bridge to the shared
presentation model and exposes default-output start/stop controls.

The provisional Windows `DjBridge` exposes GUI-thread `start`/`stop`, an active
flag and status, and emits revisioned presentation updates on the GUI thread.
Its owned worker performs endpoint discovery and analysis; a single coalescing
slot retains the latest update, including clears. The shared desktop should
connect `presentationUpdate` to `LivePresentation::apply` on the GUI thread,
bind start/stop controls and display status. It must stop, deliver the clear,
and drain or otherwise guard old updates before recreating the bridge or
presentation gate. A source revision is not a cross-process/session identity.
Stop is asynchronous; destruction joins the worker and can wait for a stalled
OS discovery call.

Windows 11 x64, MSVC 19.44, Qt 6.8.3: serial
`cmake --build "$env:TEMP/beatglass-overlay-build" --config Debug --target windows_dj_bridge_tests -j 1`
and `ctest --test-dir "$env:TEMP/beatglass-overlay-build" -C Debug -R '^windows_dj_bridge$' --output-on-failure`
passed. Qt tests exercise GUI-thread update delivery, rapid stop/start clears,
delayed-update rejection, discovery error reporting, and destruction after a
blocked discovery is released. An earlier test exposed a missing clear when a
stop arrived before the worker observed a restart; the regression now passes.
These tests use injected discovery and do not exercise the shared desktop's QML
or prove responsiveness if an OS call stalls indefinitely.
An additional opt-in `BEATGLASS_NATIVE_DJ_TEST=1` run of the same CTest target
played a generated 120 BPM click track through the default Windows output and
observed a tempo snapshot reach a GUI-thread `PresentationGate`; stopping the
bridge cleared the gate, and it remained clear after a short wait. This is a
native test of the private bridge and real capture/tempo path, not the actual
QML control or overlay. The normal and opt-in test invocations both passed.

The Windows QML integration branch connects the bridge's GUI-thread updates to
`LivePresentation` and exposes default-output start/stop controls. Windows 11
x64, MSVC 19.44 and Qt 6.8.3: serial desktop and test builds succeeded; CTest
passed `desktop_navigation`, `live_presentation`, `windows_dj_bridge` and
`windows_dj_ui` (4/4). With `BEATGLASS_NATIVE_DJ_TEST=1`, `windows_dj_ui` played
a generated click track through the default endpoint, verified that the actual
QML BPM label updated, and verified that stop cleared it. This is not an overlay
test, physical disconnect/default-switch test, real-music evaluation, or v1
qualification. The shared UI and Windows bridge PRs remain prerequisites.

Windows 11 x64, MSVC 19.44: `cmake --build "$env:TEMP/beatglass-wasapi-slice" --config Debug --target windows_endpoint_tests -j 4`
and `ctest --test-dir "$env:TEMP/beatglass-wasapi-slice" -C Debug -R 'Default follower|Discovery exceptions|Live tempo|Session|endpoint|capture startup' --output-on-failure`
passed 11/11. Opt-in `[native-default],[native-tempo]` tests passed: injected
discovery changed between two real active endpoints, and generated playback
produced a revisioned tempo snapshot followed by a clear that rejected a delayed
older snapshot through `PresentationGate`. These tests did not change the OS
default, disconnect a device, use the actual GUI delivery path, or establish
accuracy on real music. Automatic recovery after real device loss remains unverified.
The injected discovery scenario also forced an asynchronous invalid-endpoint
failure, then restored a real endpoint and observed a running source with a fresh
attempt. The last parallel MSBuild invocation failed because the Windows paging
file could not accommodate new processes; retrying the build with `-j 1` passed
the same 11 CTest cases and both opt-in native cases.

On Windows 11 Home 10.0.26300, MSVC 19.44, build `29ecc28` with the
`[native-system-default]` test added, a manual Control Panel switch changed the
multimedia default from Speakers (Realtek(R) Audio) to Speakers (Logitech G733
Gaming Headset) and back. The test observed a clear and a running follower on
each transition with fresh epochs; all 769 assertions passed. The original
multimedia default was restored, and the Speakers (2K AUDIO) communications
default remained unchanged. After a serial build of `windows_endpoint_tests`,
the opt-in command was `windows_endpoint_tests.exe '[native-system-default]' -r compact`.
This exercised actual OS default selection, not physical disconnection, real
music, or QML interaction during the switch.
