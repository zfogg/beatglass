import json
import math
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave

exe = sys.argv[1]
rate = 44100
with tempfile.TemporaryDirectory() as directory:
    path = pathlib.Path(directory) / "synthetic.wav"
    with wave.open(str(path), "wb") as wav:
        wav.setparams((1, 2, rate, 0, "NONE", "not compressed"))
        samples = bytearray()
        for i in range(rate * 12):
            t = 2 * math.pi * i / rate
            phase = i % (rate // 2)
            frequencies = (261.625565, 329.627557, 391.995436) if i < rate * 6 else (220.0, 261.625565, 329.627557)
            chord = .15 * sum(math.sin(freq * t) for freq in frequencies)
            click = .6 * math.exp(-phase / 100) * math.sin(phase * .4) if phase < 1000 else 0
            sample = max(-1, min(.999, chord + click))
            samples.extend(struct.pack("<h", int(sample * 32767)))
        wav.writeframes(samples)
    before = path.read_bytes()
    run = subprocess.run([exe, str(path)], capture_output=True, text=True, check=True)
    rows = [json.loads(line) for line in run.stdout.splitlines()]
    assert rows and rows[-1]["type"] == "result", run.stdout
    result = rows[-1]
    assert result["epoch"] > 0 and all(row["epoch"] == result["epoch"] for row in rows), rows
    assert result["schema_version"] == 1 and result["sample_rate"] == rate and result["bpm"] is not None and result["state"] == "tracking", result
    assert result["duration_seconds"] == 12.0, result
    assert all(row["duration_seconds"] == 12.0 for row in rows if "duration_seconds" in row), rows
    assert result["rolling_key"] is not None and result["local_chord"] == "A minor", result
    assert result["tonal_through"] <= result["analyzed_through"] <= result["observed_through"] == rate * 12
    beats = [row for row in rows[:-1] if row["type"] == "beat"]
    tonal = [row for row in rows[:-1] if row["type"] == "tonal"]
    assert beats and all(b["strength"] is None and b["sample_rate"] == rate and b["frame"] < b["observed_through"] <= rate * 12 for b in beats), rows
    assert tonal and all(t["sample_rate"] == rate and t["first_frame"] < t["observed_through"] <= rate * 12 for t in tonal), rows
    assert all(t["rolling_key_from"] is None or t["rolling_key_from"] <= t["first_frame"] for t in tonal), rows
    assert all((t["rolling_key"] is None) == (t["rolling_key_strength"] is None) and (t["local_chord"] is None) == (t["local_chord_strength"] is None) for t in tonal), rows
    assert any(t["local_chord"] == "C major" and t["observed_through"] < rate * 5 for t in tonal), rows
    assert any(t["local_chord"] == "A minor" and t["first_frame"] > rate * 8 for t in tonal), rows
    prefix = pathlib.Path(directory) / "prefix.wav"
    with wave.open(str(path), "rb") as source, wave.open(str(prefix), "wb") as target:
        target.setparams(source.getparams())
        target.writeframes(source.readframes(rate * 6))
    prefix_rows = [json.loads(line) for line in subprocess.run([exe, str(prefix)], capture_output=True, text=True, check=True).stdout.splitlines()]
    prior = [row for row in rows[:-1] if row["observed_through"] <= rate * 6]
    assert prefix_rows[:-1] == prior, "future PCM changed a previously emitted observation"
    quiet_path = pathlib.Path(directory) / "silence-after.wav"
    with wave.open(str(path), "rb") as source, wave.open(str(quiet_path), "wb") as target:
        target.setparams(source.getparams())
        target.writeframes(source.readframes(rate * 12) + bytes(rate * 2 * 2))
    quiet_rows = [json.loads(line) for line in subprocess.run([exe, str(quiet_path)], capture_output=True, text=True, check=True).stdout.splitlines()]
    assert any(row["type"] == "state" and row["state"] == "silent" and row["observed_through"] < rate * 14 for row in quiet_rows), quiet_rows[-10:]
    assert quiet_rows[-1]["state"] == "silent" and quiet_rows[-1]["bpm"] is None and quiet_rows[-1]["rolling_key"] is None and quiet_rows[-1]["local_chord"] is None and quiet_rows[-1]["rolling_key_strength"] is None and quiet_rows[-1]["local_chord_strength"] is None
    assert path.read_bytes() == before
    missing = subprocess.run([exe, str(path.parent / "missing.wav")], capture_output=True, text=True)
    assert missing.returncode != 0
