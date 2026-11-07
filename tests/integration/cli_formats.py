import hashlib
import pathlib
import subprocess
import sys

exe, root = sys.argv[1], pathlib.Path(sys.argv[2])
for extension in ("aiff", "flac", "mp3"):
    path = root / f"tone.{extension}"
    before = hashlib.sha256(path.read_bytes()).digest()
    result = subprocess.run([exe, "--replay", str(path)], capture_output=True, text=True)
    assert result.returncode == 0, f"{extension} decode failed: {result.stderr}"
    assert "sample rate: 44100" in result.stdout and "channels: 1" in result.stdout, result.stdout
    assert int(next(line.split(": ")[1] for line in result.stdout.splitlines() if line.startswith("frames: "))) > 0
    observed = [int(line.split()[2]) for line in result.stdout.splitlines() if line.startswith("observed frame:")]
    assert observed and observed[-1] > 0, f"{extension} decoder produced no frames"
    assert hashlib.sha256(path.read_bytes()).digest() == before, f"{extension} source was modified"
