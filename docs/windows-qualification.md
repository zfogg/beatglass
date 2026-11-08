# Windows output-monitoring qualification checklist

This checklist is required before claiming Windows DJ support. CI and offscreen Qt runs do not qualify native capture or desktop behavior.

## Build identity

The executable preparation check is `tests/windows/run_qualification.ps1`. Run it
from a PowerShell session on the target machine with `-Build` to configure and
build the pinned x64 Visual Studio configuration, or point it at an existing
build directory to repeat discovery and artifact checks:

```powershell
pwsh -File tests/windows/run_qualification.ps1 -Build -RunTests -OutputDirectory build/windows-qualification
```

The script records the commit, OS and tool versions, CTest discovery, and the
expected desktop, endpoint, and overlay test artifacts. With `-RunTests` it also executes the
discovered CTest suite and retains `ctest-run.log`. It deliberately reports the native
WASAPI, device-recovery, overlay, DPI, and interactive UI cases as `UNRUN`;
those observations still require a logged-in Windows session and evidence.

- Record commit SHA, compiler/MSVC toolset, Windows version/build, architecture, Qt and vcpkg manifest revisions.
- Build the pinned desktop preset in Release configuration and archive the exact CMake configure/build output.

## Native cases

1. Launch the desktop shell on a logged-in interactive session and confirm the DJ page opens without fallback errors.
2. Enumerate the default output device and at least one alternate output device; record names and stable identifiers.
3. Select each device, start monitoring, and verify the status transitions to running only after capture starts.
4. Play licensed test audio through the selected device and verify BPM/key/chord labels update from captured samples.
5. Switch devices and unplug/replug the active device; verify capture stops, stale labels clear, and reacquisition requires a fresh source epoch.
6. Stop monitoring and close the window; verify capture threads terminate and no device remains open.

For every case record timestamp, device, expected/observed result, logs, and a screenshot or recording. Do not claim qualification when any case is unrun or only simulated.
