# macOS qualification checklist

This checklist records native macOS audio and overlay evidence. Do not mark a case passed from a Windows build, a silent buffer, or a successful API call alone.

The repeatable build and evidence setup is `tests/macos/run_qualification.sh`. Run it from a logged-in Mac with an output-specific `NSAudioCaptureUsageDescription` that explicitly describes system-output capture and does not mention microphone or camera capture; it records the build identity, verifies launchable `APPL` bundle executable/identifier/name/version metadata, runs native test output, then prints the manual cases that still require permission, playback and desktop interaction.

For an actual output-capture run, pass the exact UID printed by the smoke bundle's discovery mode. The runner retains `output-discovery.log`, requires it to remain nonempty, rejects duplicate UIDs, and verifies any supplied UID exactly matches a discovered entry before capture:

```sh
BEATGLASS_MACOS_TAP_UID='CoreAudio-output-UID' \
  tests/macos/run_qualification.sh build/macos-qualification
```

When the logged-in Mac has no checkout, transfer the exact source tree without copying local build artifacts, then run the qualification script on the Mac:

```sh
tests/macos/prepare_remote_checkout.sh zachbook-pro /tmp/beatglass-native-qualification HEAD
ssh zachbook-pro 'cd /tmp/beatglass-native-qualification && tests/macos/run_qualification.sh build/macos-qualification'
```

The transfer helper only prepares the bounded `/tmp/beatglass-native-qualification` directory; it does not grant permissions or claim native qualification.

The qualification build enables the aubio-backed slice and the production
desktop analysis bridge, so the Mac must have
the aubio development headers installed before running the script. On a
Homebrew Mac, install the pinned formula available from the host's configured
Homebrew repository with:

```sh
brew install aubio
```

On Intel Macs, current Homebrew may have no aubio dependency bottles and can
build NumPy and aubio from source. The install can therefore take many
minutes; verify completion with `brew list --versions aubio` and locate the
header with `find "$(brew --prefix aubio)" -name aubio.h` before rerunning the
qualification script. Do not treat an active Homebrew build or a formula
prefix printed by `brew --prefix aubio` as proof that aubio is installed.

The runner performs this header preflight and stops with the same remediation
if `aubio/aubio.h` is absent. A missing header is a dependency blocker, not a
native qualification result; record the failed command and rerun after the
dependency is installed.

The runner enables `BEATGLASS_BUILD_LIBRARY_PROTOTYPE=ON` because the shared
test target includes storage tests that require the prototype library. Without
that option, configuration can succeed but the test executable fails to link.

Set `BEATGLASS_MACOS_PERMISSION_RESULT=allowed` or `denied` after observing the System Settings prompt/revocation; `allowed` additionally requires `BEATGLASS_MACOS_TAP_UID`. Omit it to record `UNRUN` without changing permissions.

Set `BEATGLASS_MACOS_REQUIRE_SIGNING=1` for a release qualification run. The runner then fails closed unless the production desktop, tap-smoke, and recovery bundles all pass `codesign --verify --deep --strict`; the default remains permissive so unsigned local development builds can still produce diagnostic evidence.

The runner invokes both the tap and recovery smoke bundles when this variable is set; when it is absent, the log records capture as `UNRUN`. A supplied-UID run retains `tap-smoke.log` and `recovery-smoke.log` and fails unless tap output includes nonzero frames/samples, discontinuity and dropped-frame counters, and `timestamp_valid=1`; both recovery cycles must report nonzero frames/samples, discontinuity counts, and successful stops. Overlay/native CTest discovery is retained as `overlay-ctest-discovery.log`; execution output is retained as `overlay-ctest.log` and must contain a passing percentage summary. The exact configure/build/test commands are retained as `qualification-commands.log`; `qualification-manifest.txt` summarizes commit, macOS version, architecture, Xcode version, SDK version/path, active developer directory, signing state for desktop/tap/recovery, UID mode, evidence paths, and the path to `qualification-artifacts.sha256`. That SHA-256 manifest contains one hash per native desktop, tap-smoke, and recovery executable; preserve it with the logs and verify it later with `shasum -a 256 -c qualification-artifacts.sha256`. Codesign logs retain signing metadata for all three bundles. The runner also fails if required evidence files are empty.

After transferring a qualification output directory, verify its evidence without rerunning native tests:

```sh
tests/macos/verify_qualification_artifacts.sh /path/to/qualification-output
```

The verifier requires the manifest identity fields, exactly three relative artifact paths, rejects duplicate or path-escaping entries, and recomputes successful SHA-256 checks for the desktop, tap-smoke, and recovery binaries. The runner writes paths relative to the qualification output so the directory can be transferred and verified on another Mac. It also emits `qualification-cases.tsv`, a fixed ten-case matrix initialized to `UNRUN`; update statuses and evidence paths only after observing each native scenario, then rerun the verifier. For `PASS` rows, the verifier requires a relative evidence path that exists inside the transferred qualification output.

## Build identity

Record the commit, macOS version, hardware model, Xcode and SDK versions, Qt version, signing state, selected output UID and display arrangement. Record whether the run used the production desktop bundle or a smoke bundle, plus the exact configure, build and CTest commands.

## Permission and capture

1. Start the signed bundle with `NSAudioCaptureUsageDescription` present. Record whether macOS shows the system-audio permission prompt.
2. Allow access in **System Settings → Privacy & Security → Screen & System Audio Recording**. Relaunch the same bundle and record the listed application identity.
3. Play a generated tone through the selected output from a separate process. Record frames, nonzero samples, FIFO drops, discontinuities, timestamp validity and exit status.
4. Repeat after selecting another output. Verify the selected UID and that no microphone or unrelated output is used.
5. Deny access, revoke it in System Settings, and restore it. Record the typed error or unknown state, stale-evidence clearing, and whether a fresh start recovers.

## Callback fault and FIFO exhaustion

During the tone run, deliberately let the control worker stop draining long
enough to fill the bounded FIFO, then resume polling. Record the nonzero
`dropped_frames()` count, the discontinuity on the first accepted block after
the drop, and the `status()` transition to `interrupted` when the FIFO reaches
its exhaustion limit. The control owner must stop the tap, discard queued
blocks, invalidate live analysis, and restart with a fresh epoch; it must not
continue presenting the old epoch after a callback fault. Also record malformed
or unexpected callback buffers as `UNRUN` unless the native harness can
exercise them safely; synthetic FIFO tests do not qualify the Core Audio IOProc.

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

## Native access limitation

Current qualification status is `UNRUN` with `NATIVE_MAC_REQUIRED=1`. The `zachbook-pro` Tailscale host is reachable (macOS 26.7.2, x86_64), but it has no Beatglass checkout or native qualification build available to the runner. The portable Windows workspace still cannot provide the required macOS Core Audio, Qt/AppKit, Screen & System Audio Recording, or overlay behavior, so no native result is inferred from the SSH connectivity check alone.

The remaining native cases are: permission prompt, allow/deny/revoke recovery, generated-tone output capture with nonzero frames and valid timestamps, output switching and UID evidence, tap/recovery smoke, and the interactive overlay/menu-bar/display/click-through/fullscreen/Spaces scenarios above. Do not mark these cases `PASS` until evidence is captured on the intended signed macOS production build.

Portable checks completed for this documentation update are `git diff --check` and `bash -n tests/macos/run_qualification.sh`; neither substitutes for native qualification.
