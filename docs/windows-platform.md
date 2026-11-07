# Windows platform work

The provisional backend enumerates active render endpoints and opens a selected
endpoint with WASAPI shared-mode loopback. The experimental live tempo bridge is
currently disconnected from the desktop DJ page pending capture-age and
post-analysis loss checks. Earlier integration observations below are historical.
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

The experimental Windows DJ page now owns a separate tempo overlay and uses
the merged overlay controller. The main window provides show/hide, click-through
unlock, opacity and on-screen recovery controls. The overlay displays the same
live BPM model as the DJ page; key, chords and beat phase are not implemented.
Windows 11 10.0.26300, MSVC 19.44 and Qt 6.8.3: a serial build of
`beatglass-desktop`, `windows_dj_ui_tests` and `windows_overlay_tests` passed.
With the pinned vcpkg debug and release DLL directories on `PATH`,
`ctest --test-dir "$env:TEMP/beatglass-overlay-build" -C Debug -R '^(windows_dj_ui|windows_overlay|windows_dj_bridge|desktop_navigation|live_presentation)$' --output-on-failure`
passed 5/5. The UI integration test operates the QML controls and checks the
real controller, window visibility, click-through, opacity and recovery. The
first run without those DLL paths timed out on two test launches; the rerun
passed. This is automated integration coverage, not a manual product overlay,
physical monitor removal or exclusive fullscreen qualification.

On Windows 11 Home 10.0.26300, MSVC 19.44, Qt 6.8.3, desktop build based on
`8df919d` with the drag changes, interactive testing showed the overlay over the
DJ controls. The opacity slider visibly changed transparency. With click-through
enabled, clicks on covered Start and Stop monitoring controls reached them;
after Unlock, a click on the covered Start control did not reach it. The
control-window labels tracked the state. The application was closed after the
test. The unlocked overlay now has a drag surface and starts near the control
window. The opt-in `BEATGLASS_NATIVE_OVERLAY_TEST=1` invocation
`windows_dj_ui_tests.exe nativeOverlayDrag` passed with native pointer input and
observed a position change on one display. An earlier run did not move the
window, and a second-display version failed converting a point after changing
screens. The drag surface now tracks pointer movement in Qt global coordinates;
the native test reads the Windows window rectangle for pointer input. On build
`38aad62` with these changes, an interactive desktop drag moved the overlay 240
by 100 logical pixels. Three consecutive native `windows_dj_ui_tests.exe
nativeOverlayDrag` runs passed on both LG ULTRAGEAR (100% scaling) and U2790B
(150% scaling), checking a substantial position change on each screen. The five
targeted CTest cases passed again. This covers drags started and completed
within each display; a single drag crossing the monitor boundary, physical
display removal and exclusive fullscreen product scenarios remain untested.

Follow-up on build `7308514` with the mixed-DPI and settings changes: native
pointer input dragged the QML overlay continuously from the 100% LG display to
the 150% U2790B. Three consecutive boundary runs passed, and a final run checked
both the crossed boundary and movement within each display; the test verifies
the destination monitor and that the window still contains the pointer. Earlier
attempts failed because the overlay stopped following the pointer during the
DPI transition. The Windows controller now follows the native cursor until
button release, including if Qt mouse events are interrupted. An initial
per-display test also placed the second window on the wrong monitor; the test
now checks actual native monitor membership before dragging.

Position and opacity are now persisted through `QSettings`, restored to an
available display, and reflected in the opacity slider. Visibility and
click-through start disabled. Unit and QML restart tests use temporary settings;
the actual desktop was also closed while default-output monitoring and
click-through were active, then reopened with monitoring stopped, the overlay
hidden and its previously selected opacity restored. The five targeted CTest
cases passed with the required vcpkg DLL paths. The session epoch namespace is
not yet shared by the live worker and offline sources; see
`epoch-handoff-proposal.md`. Physical monitor removal, the reverse boundary
direction, exclusive fullscreen product behavior and full native qualification
remain open.

Following merged allocator PR #63, the provisional `DjBridge` requires an
injected process-session `SourceEpochAllocator`; its worker no longer creates a
private allocator. The application owner must retain the same allocator across
live and library sources until both workers have stopped and joined. The shared
desktop does not instantiate this bridge in this branch. Noninteractive tests
cover a generated file reader's coordinator-issued initial epoch alongside
failed live starts and restarts, and allocation across bridge recreation. The
legacy file reader seek still increments locally and must not be used in a
shared epoch namespace; coordinated seeks require the explicit-epoch reader
contract proposed in PR #45. Full live/library integration remains open.

Queue-overflow review found that clearing tempo evidence alone allowed queued
pre-gap PCM to be consumed after the clear. On observing dropped frames the
session now fails the attempt, invalidates evidence, stops capture and discards
the backlog; a later restart obtains a fresh epoch. A control-thread fake
backend regression covers both a drop before a pop and a drop during the pop,
rejects queued pre-gap packets, and accepts a fresh-epoch packet after restart.
Windows 11 10.0.26300 x64, MSVC 19.44, existing pinned vcpkg libraries: serial
`cmake --build "$env:TEMP/beatglass-overlay-build" --config Debug --target windows_endpoint_tests windows_dj_bridge_tests -j 1`
and the targeted CTest expression `Queue overflow invalidates|Session|Default follower|Live tempo|Discovery exceptions|capture startup|shared epoch allocator|shared epoch exhaustion|^windows_dj_bridge$`
passed 11/11. This deterministic test does not reproduce hardware queue
overflow. The default follower can retry after failure; selected-output UI
recovery remains an integration behavior to verify.

The shared provisional tray/menu and per-user relaunch handoff keep controls discoverable until a reliable Windows recovery route is established. Qt's reported tray availability and icon visibility do not prove reachability across Explorer loss or restart; no hidden-window guarantee is claimed. The external recovery contract unlocks the overlay when a verified route is revoked while controls are unavailable. Offscreen controller tests do not qualify Explorer, taskbar, pointer, or desktop behavior. The earlier Qt Windows platform-initialization crash and interrupted `windows_dj` run remain unverified.

The merged WASAPI device-position checks mark jumps, resets, invalid timestamps and flagged discontinuities for downstream reacquisition. Capture validates frame and timestamp arithmetic before publication; overflow releases the buffer and interrupts the attempt. Empty invalid or flagged buffers carry a discontinuity to the next nonempty packet. These checks have deterministic unit coverage; physical endpoint changes, driver overflow and integrated real-DJ reacquisition remain unqualified.

Device-position continuity checks are deterministic unit coverage only. Physical
endpoint switching, device removal and return, and integrated reacquisition over
actual DJ output remain unqualified.

Capture validates epoch-frame and valid QPC arithmetic for the complete WASAPI
buffer before publishing a chunk. Arithmetic overflow releases that buffer and
interrupts the attempt; invalid timestamps are not added. Empty invalid or
flagged buffers retain a discontinuity for the next nonempty packet. Boundary
checks are simulated; actual driver overflow and empty flagged buffers have not
been observed or qualified.

The provisional composite worker can use a shared one-attempt start
authorization: Stop or replacement cancels it without waiting for the worker,
and only one worker can claim it. A pre-claim cancellation prevents the start;
after a claim, cancellation is checked before and after the audio start call
and during capture polling; earlier initialization calls are not individually
checked. A native call already in progress is not preempted. A racing poll or
pop may return a packet, so synchronized authorization or generation gating
is required before publishing. The desktop bridge must retain and cancel its own authorization on
Stop, switch and destruction, gate stale deliveries, and join the worker. This
contract has deterministic fake-backend coverage; the bridge integration and
physical device timing remain unqualified.

Selected and default Windows tempo paths now reject untimed, future, and
over-one-second-old QPC packets, and check session loss and packet age after
DSP before publishing an observed boundary or BPM. A capture-clock failure
clears evidence and fails the attempt. Windows 11 10.0.26300 x64/MSVC 19.44
fake-backend tests cover age boundaries, post-DSP interruption, aging and clock
failure; native WASAPI timestamp compatibility and actual DJ output remain
unqualified. Consumers still need synchronized delivery gates.

The provisional live composite consumer rejects untimed, future, and more than
one second old QPC capture packets. It checks age before and after DSP and
polls the capture session again before publishing each result. Deterministic
fake-backend tests exercise queued stale PCM and loss during DSP. A failed
capture clock fails the attempt. Native WASAPI QPC behavior and production
latency are not yet qualified; generation/token gates remain necessary for
loss or cancellation racing the final check.
