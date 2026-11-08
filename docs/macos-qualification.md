# macOS qualification checklist

This checklist records native macOS audio and overlay evidence. Do not mark a case passed from a Windows build, a silent buffer, or a successful API call alone.

The repeatable build and evidence setup is `tests/macos/run_qualification.sh`. Run it from a logged-in Mac with an output-specific `NSAudioCaptureUsageDescription` that explicitly describes system-output capture and does not mention microphone or camera capture; it records the build identity, verifies launchable `APPL` bundle executable/identifier/name/version metadata, runs native test output, then prints the manual cases that still require permission, playback and desktop interaction.

For an actual output-capture run, pass the exact UID printed by the smoke bundle's discovery mode. The runner retains `output-discovery.log`, rejects duplicate UIDs, and verifies any supplied UID exactly matches a discovered entry before capture:

```sh
BEATGLASS_MACOS_TAP_UID='CoreAudio-output-UID' \
  tests/macos/run_qualification.sh build/macos-qualification
```

Set `BEATGLASS_MACOS_PERMISSION_RESULT=allowed` or `denied` after observing the System Settings prompt/revocation; `allowed` additionally requires `BEATGLASS_MACOS_TAP_UID`. Omit it to record `UNRUN` without changing permissions.

The runner invokes both the tap and recovery smoke bundles when this variable is set; when it is absent, the log records capture as `UNRUN`. A supplied-UID run retains `tap-smoke.log` and `recovery-smoke.log` and fails unless tap output includes nonzero frames/samples, discontinuity and dropped-frame counters, and `timestamp_valid=1`; both recovery cycles must report nonzero frames/samples, discontinuity counts, and successful stops. Overlay/native CTest discovery is retained as `overlay-ctest-discovery.log`; execution output is retained as `overlay-ctest.log` and must contain a passing percentage summary. The exact configure/build/test commands are retained as `qualification-commands.log`; `qualification-manifest.txt` summarizes commit, macOS version, architecture, Xcode version, SDK version/path, active developer directory, signing state for desktop/tap/recovery, UID mode, and evidence paths, while codesign logs retain signing metadata for all three bundles. The runner also fails if required evidence files are empty.

## Build identity

Record the commit, macOS version, hardware model, Xcode and SDK versions, Qt version, signing state, selected output UID and display arrangement. Record whether the run used the production desktop bundle or a smoke bundle, plus the exact configure, build and CTest commands.

## Permission and capture

1. Start the signed bundle with `NSAudioCaptureUsageDescription` present. Record whether macOS shows the system-audio permission prompt.
2. Allow access in **System Settings → Privacy & Security → Screen & System Audio Recording**. Relaunch the same bundle and record the listed application identity.
3. Play a generated tone through the selected output from a separate process. Record frames, nonzero samples, FIFO drops, discontinuities, timestamp validity and exit status.
4. Repeat after selecting another output. Verify the selected UID and that no microphone or unrelated output is used.
5. Deny access, revoke it in System Settings, and restore it. Record the typed error or unknown state, stale-evidence clearing, and whether a fresh start recovers.

The tap path is scoped to macOS 14.2 or later and the selected output stream. The smoke binary exits unsuccessfully if it observes frames/samples but never receives a valid native sample timestamp. Silence or a successful start does not prove permission. See [Apple’s Core Audio tap reference](https://developer.apple.com/documentation/coreaudio/capturing-system-audio-with-core-audio-taps).

## Overlay and recovery

1. Launch the production bundle with visible controls. Show the overlay, move it, change opacity and select each display.
2. Enable click-through and verify the overlay receives no pointer events while controls or a verified recovery route remain available.
3. Hide, minimize, close and destroy controls. Verify the overlay unlocks or hides when recovery is unavailable.
4. Exercise the menu-bar item: show controls, unlock/edit, hide overlay and quit. Reopen from the Dock and verify the existing instance is restored rather than duplicated.
5. Remove or change a display, work area or DPI. Verify the overlay is clamped and the selected display refreshes.
6. Test over a fullscreen or Spaces window and record whether the overlay remains visible; AppKit behavior is a request, not a guarantee.

## Evidence rules

Record `PASS`, `FAIL`, or `UNRUN` for each case with timestamp, action or command, observed output, and log or screenshot path. `UNRUN` is correct while the Mac is unavailable or interactive testing is not authorized. Qualification is complete only when every required case has evidence on the intended production build.
