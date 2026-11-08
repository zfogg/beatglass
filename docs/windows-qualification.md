# Windows output-monitoring qualification checklist

This checklist is required before claiming Windows DJ support. CI and offscreen Qt runs do not qualify native capture or desktop behavior.

The runner also writes `qualification-manifest.txt` with required build identity
and canonical evidence links; the offline verifier rejects missing, duplicate,
or redirected identity fields.

## Build identity

The executable preparation check is `tests/windows/run_qualification.ps1`. Run it
from a PowerShell session on the target machine with `-Build` to configure and
build the pinned x64 Visual Studio configuration, or point it at an existing
build directory to repeat discovery and artifact checks:

```powershell
pwsh -File tests/windows/run_qualification.ps1 -Build -RunTests -OutputDirectory build/windows-qualification
```

The script records the commit, OS and tool versions, CTest discovery, and the
expected desktop, endpoint, DJ bridge, and overlay test artifacts. It also emits
`qualification-artifacts.sha256`, containing a SHA-256 entry for every required
executable with paths relative to the qualification output. Preserve that file
with the logs and verify it later without rebuilding or rerunning native tests:

```powershell
pwsh -File tests/windows/verify_qualification_artifacts.ps1 build/windows-qualification
```

The verifier requires exactly four relative artifact paths, rejects duplicate or
path-escaping entries, recomputes every SHA-256 digest, and requires the four
expected `build/Release` desktop, endpoint, DJ bridge, and overlay executables.
With `-RunTests` it also executes the
discovered CTest suite and retains `ctest-run.log`. It deliberately reports the native
WASAPI, device-recovery, overlay, DPI, and interactive UI cases as `UNRUN`;
those observations still require a logged-in Windows session and evidence.
The runner also emits `native-playback-evidence.json`, a fixed six-case matrix
initialized to `UNRUN`; update statuses and log/screenshot fields only after
observing each native playback scenario, then rerun the verifier before claiming
qualification.
For `PASS` rows, the verifier requires relative log and screenshot paths that
exist inside the transferred qualification output, preventing status-only or
external-path claims.
It emits `native-overlay-evidence.json` for the four overlay recovery, click-through,
display/DPI, and fullscreen cases; a `PASS` requires screenshot evidence and is
rejected by the verifier when that field is missing.

- Record commit SHA, compiler/MSVC toolset, Windows version/build, architecture, Qt and vcpkg manifest revisions.
- Build the pinned desktop preset in Release configuration and archive the exact CMake configure/build output.

## Native cases

1. Launch the desktop shell on a logged-in interactive session and confirm the DJ page opens without fallback errors.
2. Enumerate the default output device and at least one alternate output device; record names and stable identifiers.
3. Select each device, start monitoring, and verify the status transitions to running only after capture starts.
4. Play licensed test audio through the selected device and verify BPM/key/chord labels update from captured samples.
5. Switch devices and unplug/replug the active device; verify capture stops, stale labels clear, and reacquisition requires a fresh source epoch.
6. Stop monitoring and close the window; verify capture threads terminate and no device remains open.

## Native file playback cases

7. On the playback panel, refresh output devices, select an endpoint, enter the
   decoded source sample rate/channel count, and start native playback. Record
   the endpoint and confirm the UI reports `Native output active` only after
   the device opens.
8. Pause and resume the same source; confirm output stops while paused and
   resumes without accepting stale frames. Seek or change source and verify the
   previous native device is retired before restarting on a fresh epoch.
9. Remove or disable the selected output while playback is active, refresh the
   device list, and verify playback stops with an actionable recovery message.
   Select a replacement endpoint and use Retry native; record successful and
   failed-start diagnostics.

For every case record timestamp, device, expected/observed result, logs, and a screenshot or recording. Do not claim qualification when any case is unrun or only simulated.

The runner creates `native-playback-evidence.json` with one `UNRUN` record per case. Complete that template during a logged-in Windows session by recording the selected device, observed result, log evidence, and screenshot path; retain it with the artifact hash manifest. The template is evidence collection only and never marks a case passed automatically.
