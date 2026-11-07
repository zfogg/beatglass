import math
import pathlib
import re
import struct
import subprocess
import sys
import tempfile
import wave

exe = sys.argv[1]
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    def make(path, clicks, stereo=False):
        with wave.open(str(path), "wb") as wav:
            wav.setparams((2 if stereo else 1, 2, 44100, 0, "NONE", "not compressed"))
            frames = bytearray()
            for i in range(44100 * 20):
                phase = i % 22050
                value = int(28000 * math.exp(-phase / 100) * math.sin(phase * .4)) if clicks and phase < 1000 else 0
                frames.extend(struct.pack("<hh", value, -value) if stereo else struct.pack("<h", value))
            wav.writeframes(frames)
    silence = root / "silence.wav"
    clicks = root / "clicks.wav"
    stereo = root / "anti-phase.wav"
    make(silence, False); make(clicks, True); make(stereo, True, True)
    quiet = subprocess.run([exe, str(silence)], capture_output=True, text=True, check=True).stdout
    assert "format: WAV" in quiet and "sample rate: 44100" in quiet and "tempo: unknown" in quiet, quiet
    result = subprocess.run([exe, str(clicks)], capture_output=True, text=True, check=True).stdout
    assert "tempo: unknown" not in result and "BPM" in result, result
    bpm = float(re.search(r"tempo: ([0-9.]+) BPM", result).group(1))
    assert any(abs(bpm - expected) < expected * .08 for expected in (60, 120, 240)), result
    anti = subprocess.run([exe, str(stereo)], capture_output=True, text=True, check=True).stdout
    assert "tempo: unknown" not in anti and "channels: 2" in anti, anti
    replay = subprocess.run([exe, "--replay", str(clicks)], capture_output=True, text=True, check=True).stdout
    lines = [line for line in replay.splitlines() if line.startswith("observed frame:")]
    assert lines and "tempo: unknown" in lines[0], replay
    assert [int(line.split()[2]) for line in lines] == sorted(int(line.split()[2]) for line in lines)
    assert int(lines[-1].split()[2]) == 44100 * 20
    prefix_frames = (44100 * 8 // 512) * 512
    prefix = root / "prefix.wav"
    with wave.open(str(clicks), "rb") as source, wave.open(str(prefix), "wb") as target:
        target.setparams(source.getparams())
        target.writeframes(source.readframes(prefix_frames))
    prefix_output = subprocess.run([exe, "--replay", str(prefix)], capture_output=True, text=True, check=True).stdout
    prefix_lines = [line for line in prefix_output.splitlines() if line.startswith("observed frame:")]
    assert prefix_lines == lines[:len(prefix_lines)], "future samples changed an earlier estimate"
    assert result.split("tempo: ")[-1] == replay.split("tempo: ")[-1], (result, replay[-500:])
    missing = subprocess.run([exe, str(root / "missing.wav")], capture_output=True, text=True)
    assert missing.returncode != 0
