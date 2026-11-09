# PipeWire Linux qualification runbook

Run from a Linux checkout with the PipeWire development package and a live
user daemon:

```sh
pkg-config --modversion libpipewire-0.3
cmake -S . -B build-pipewire -DBEATGLASS_BUILD_PIPEWIRE=ON
cmake --build build-pipewire
ctest --test-dir build-pipewire --output-on-failure -R pipewire
# The runner also invokes the daemon-backed pipewire_tests target explicitly.
# Or run the complete configure/build/CTest pass with a JSON summary:
python tests/pipewire/run_linux_native_qualification.py --summary evidence/native-summary.json
# Optional machine-readable artifact summary:
python tests/pipewire/validate_qualification_artifacts.py <artifact-dir> --json
```

Record the commit, compiler, PipeWire version, sink serial, negotiated format,
and daemon status in the evidence template. Exercise discovery, stream start,
process delivery, callback error, stop/quiesce, restart, and source
reacquisition. Attach logs or recordings for each case; headless CI does not
replace native desktop/audio qualification.

The runner JSON records native_pipewire_tests separately; qualification requires that target to return zero.
It also records the manifest capture command's stdout/stderr alongside the
native command logs.
