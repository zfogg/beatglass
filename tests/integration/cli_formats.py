import hashlib
import pathlib
import subprocess
import sys

exe, root = sys.argv[1], pathlib.Path(sys.argv[2])
help_result = subprocess.run([exe, "--help"], capture_output=True, text=True)
assert help_result.returncode == 0 and "Usage: beatglass-tool" in help_result.stdout
version_result = subprocess.run([exe, "--version"], capture_output=True, text=True)
assert version_result.returncode == 0 and version_result.stdout.startswith("beatglass-tool ")
for extension in ("wav", "aiff", "flac", "mp3"):
    path = root / f"tone.{extension}"
    before = hashlib.sha256(path.read_bytes()).digest()
    result = subprocess.run([exe, "--replay", str(path)], capture_output=True, text=True)
    assert result.returncode == 0, f"{extension} decode failed: {result.stderr}"
    assert "sample rate: 44100" in result.stdout and "channels: 1" in result.stdout, result.stdout
    frames = int(next(line.split(": ")[1] for line in result.stdout.splitlines() if line.startswith("frames: ")))
    duration = float(next(line.split(": ")[1] for line in result.stdout.splitlines() if line.startswith("duration seconds: ")))
    assert frames > 0 and duration > 0 and abs(duration - frames / 44100) < 1e-6
    observed = [int(line.split()[2]) for line in result.stdout.splitlines() if line.startswith("observed frame:")]
    assert observed and observed[-1] > 0, f"{extension} decoder produced no frames"
    assert hashlib.sha256(path.read_bytes()).digest() == before, f"{extension} source was modified"
