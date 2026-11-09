from pathlib import Path
p = (Path(__file__).parents[2] / "docs/pipewire-linux-evidence-template.md").read_text()
for marker in ("Host/build identity", "PipeWire version", "Sink serial", "Process callback evidence", "Error/stop/reacquire evidence", "CTest command"):
    assert marker in p
print("PipeWire Linux evidence template verified (6 checks)")
