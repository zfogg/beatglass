# Desktop packaging evidence

Beatglass does not currently claim a distributable installer. A native
qualification build must nevertheless retain enough evidence to distinguish a
working development tree from a package candidate.

Packaging metadata is opt-in. Configure a desktop build with
`-DBEATGLASS_ENABLE_PACKAGING=ON`, run `cmake --install <build> --prefix
<staging>`, and use `cpack --config <build>/CPackConfig.cmake` to produce the
platform archive. The install step is intentionally limited to the desktop
target; dependency deployment, signing, notarization, and clean-machine tests
remain required gates below.

## Opt-in package metadata

Release candidates can generate a package archive with CPack by configuring
with `-DBEATGLASS_ENABLE_PACKAGING=ON` and running `cpack` from the build
directory. The project supplies reproducible package identity and platform
defaults (ZIP on Windows, DragNDrop on macOS, TGZ elsewhere). This is package
metadata and archive generation only: installer signing, notarization,
dependency bundling, update/rollback, and clean-machine installation remain
separate release gates and must not be marked `PASS` from a local CPack run.

## Windows

Run `tests/windows/run_qualification.ps1 -Build -RunTests -OutputDirectory
build/windows-qualification` on the target Windows machine. The runner invokes
`windeployqt` through the desktop target, requires the desktop, endpoint, DJ
bridge, and overlay test executables, and writes `qualification-artifacts.sha256`.
Preserve that manifest with `qualification.log`, `ctest-run.log`, and the
configure/build output. Verify each retained file with PowerShell:

```powershell
Get-FileHash -Algorithm SHA256 .\build\windows-qualification\build\Release\beatglass-desktop.exe
```

This is artifact evidence, not installer signing or MSI/package evidence.

## macOS

Run `tests/macos/run_qualification.sh` on a logged-in Mac. The runner validates
the three `APPL` bundles, records codesign output, and writes
`qualification-artifacts.sha256` for the desktop, tap-smoke, and recovery
executables. Preserve it with `qualification-manifest.txt` and the native logs;
verify it later with:

```sh
shasum -a 256 -c qualification-artifacts.sha256
```

An ad-hoc or locally signed bundle is not a release package. Installer signing,
notarization, update/rollback, and clean-machine installation remain explicit
release gates and must not be marked `PASS` from these qualification scripts.
