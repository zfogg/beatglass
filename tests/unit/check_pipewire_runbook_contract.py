from pathlib import Path
p = (Path(__file__).parents[2] / "docs/pipewire-linux-qualification-runbook.md").read_text()
for marker in ("pkg-config --modversion libpipewire-0.3", "BEATGLASS_BUILD_PIPEWIRE=ON", "ctest", "stop/quiesce", "source reacquisition"):
    assert marker in p
print("PipeWire Linux qualification runbook verified (5 checks)")
