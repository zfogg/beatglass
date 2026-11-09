# PipeWire Linux qualification checklist

The Windows worktree validates only source contracts. Before claiming native
PipeWire support, a Linux host must record:

1. `pkg-config --modversion libpipewire-0.3` and the resolved
   `pipewire/pipewire.h` include path.
2. Successful `BEATGLASS_BUILD_PIPEWIRE=ON` configure/build with the
   `pw_stream` owner compiled under `BEATGLASS_HAS_PIPEWIRE=1`.
3. A live PipeWire daemon and a real output sink selected by serial.
4. Native stream format negotiation, process callback delivery into the
   bounded `MonitorStream`, callback error propagation, stop/quiesce, and
   fresh-epoch reacquisition.
5. CTest results plus captured logs/evidence for each scenario.

Until those steps are run on Linux, native PipeWire qualification is unrun.

## Adapter seam preflight

Run `ctest --test-dir build -R pipewire_linux_native_prerequisites
--output-on-failure` first. On Windows this is a safe pending preflight; on
Linux it verifies `pkg-config` can resolve `libpipewire-0.3` and gives the
install/configure guidance when headers are missing.

Before the native run, configure with `-DBUILD_TESTING=ON` and run
`ctest --test-dir build -R pipewire_linux_adapter_seam_contract --output-on-failure`.
This verifies that the Linux-gated PipeWire/SPA translation unit, callback
ownership boundary, and provisional format/target-node contract are present.
It does not replace a Linux build with `libpipewire-0.3`, a running daemon, or
real capture/recovery evidence.

The preflight requires an addressable output sink with a numeric object id,
stable serial, and node name; record the selected serial and id in the native
qualification manifest before capture.

The preflight validates the complete sink set before qualification: every
Audio/Sink must have a non-negative numeric object id, a non-empty unique
`object.serial`, and a non-empty unique `node.name`. This prevents an
ambiguous inventory from silently selecting the wrong output.

`pipewire_tests` also exercises the portable source-adapter failure boundary;
native daemon-backed lifecycle cases remain skipped/pending until Linux.
Run `ctest --test-dir build -R pipewire_tests --output-on-failure` during the
Linux qualification pass and retain the test output with the manifest.
Capture the runtime node inventory with
`python tests/pipewire/capture_pipewire_inventory.py <evidence>/pw-dump.json`.
For a single immutable artifact containing the raw inventory, UTC capture time,
host/Python identity, and `pw-cli` version, use
`python tests/pipewire/capture_pipewire_manifest.py <evidence>/manifest.json`.
Validate a copied artifact before review with
`python tests/pipewire/validate_pipewire_manifest.py <evidence>/manifest.json`.
Validation also requires every raw inventory entry to remain a JSON object.
The capture timestamp must be timezone-qualified ISO-8601 so evidence from
different hosts can be ordered reliably.
The validator requires the offset to be UTC (`+00:00`), matching the capture
command and avoiding local-time ambiguity.
Validation refuses symlinked manifest paths so evidence review cannot silently
follow an unrelated file.
The manifest host identity must begin with `Linux`; Windows capture remains a
safe pending path and cannot be mistaken for native qualification evidence.
Every raw inventory object must also retain its non-negative numeric PipeWire
object ID for later target selection and audit; IDs must be unique within the
captured inventory.
An accepted native manifest must contain at least one captured PipeWire object;
an empty dump is treated as failed qualification evidence.
At least one captured object must be an `Audio/Sink`; unrelated nodes alone do
not provide native output evidence.
Each object must retain its `info` object so native properties remain
auditable after capture, including its `props` dictionary.
Manifest publication uses a temporary file and atomic no-overwrite link, so an
interrupted or racing capture cannot leave a partial artifact at the requested
path.
Capture paths are immutable: choose a new output filename for each run. The
helper writes the dump to a temporary file and publishes it with an atomic
no-overwrite hard link, then removes the temporary file; it refuses to
overwrite existing files or traverse symlinked directories, including when a
second qualification process races the preflight check.
