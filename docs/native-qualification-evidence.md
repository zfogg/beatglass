# Native qualification evidence contract

Windows and macOS qualification outputs are transferable bundles. Keep the
following identity and machine-observation files together with the logs and
screenshots; a status without its corresponding file is not evidence.

| Evidence | Windows | macOS | Purpose |
| --- | --- | --- | --- |
| Build identity | `qualification-manifest.txt` | `qualification-manifest.txt` | commit, OS, architecture, toolchain, Qt and hardware identity |
| Binary identity | `qualification-artifacts.sha256` | `qualification-artifacts.sha256` | hashes of the exact production/smoke executables |
| Display inventory | `display-topology.json` | `display-topology.json` | monitor/adapter topology captured before overlay tests |
| Audio inventory | `audio-endpoints.json` | `audio-endpoints.json` | endpoint inventory and capture timestamp |
| Playback cases | `native-playback-evidence.json` | `qualification-cases.tsv` | case status plus observed evidence references |
| Overlay cases | `native-overlay-evidence.json` | `qualification-cases.tsv` | visibility, recovery, DPI/display and interaction evidence |

Run the platform verifier after transferring a bundle. It validates paths,
hashes, identity fields, machine inventories and that `PASS` rows point to
non-empty evidence inside the bundle. `UNRUN` is the correct state when a
logged-in native session, permission, device or display is unavailable.
