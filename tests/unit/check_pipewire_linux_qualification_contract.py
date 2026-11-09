from pathlib import Path
p = (Path(__file__).parents[2] / "docs/pipewire-linux-qualification.md").read_text()
for marker in ("pkg-config --modversion libpipewire-0.3", "pw_stream", "bounded `MonitorStream`", "fresh-epoch reacquisition"):
    assert marker in p
print("PipeWire Linux qualification checklist verified (4 checks)")
