import math
import pathlib
import re
import struct
import subprocess
import sys
import tempfile
import wave

exe = sys.argv[1]
missing_argument = subprocess.run([exe], capture_output=True, text=True)
assert missing_argument.returncode == 2 and "Usage: beatglass-tool" in missing_argument.stdout
extra_argument = subprocess.run([exe, "one.wav", "two.wav"], capture_output=True, text=True)
assert extra_argument.returncode == 2 and "Usage: beatglass-tool" in extra_argument.stdout
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
    def causal_fields(line):
        # Duration is metadata for the complete input, not a causal analysis field.
        return re.sub(r" duration seconds: [0-9.]+", "", line)
    assert [causal_fields(line) for line in prefix_lines] == [
        causal_fields(line) for line in lines[:len(prefix_lines)]
    ], "future samples changed an earlier estimate"
    assert result.split("tempo: ")[-1] == replay.split("tempo: ")[-1], (result, replay[-500:])
    transition_file = root / "silence-after-clicks.wav"
    with wave.open(str(clicks), "rb") as source, wave.open(str(transition_file), "wb") as target:
        target.setparams(source.getparams())
        target.writeframes(source.readframes(44100 * 20) + bytes(44100 * 2 * 3))
    transition_output = subprocess.run([exe, "--replay", str(transition_file)], capture_output=True, text=True, check=True).stdout
    transition_lines = [line for line in transition_output.splitlines() if line.startswith("observed frame:")]
    assert any("state: tracking" in line for line in transition_lines), "never acquired"
    assert "tempo: unknown state: silent" in transition_lines[-1], transition_lines[-1]
    resume_file = root / "resume.wav"
    with wave.open(str(clicks), "rb") as source, wave.open(str(resume_file), "wb") as target:
        target.setparams(source.getparams())
        click_frames = source.readframes(44100 * 20)
        target.writeframes(click_frames + bytes(44100 * 2 * 3) + click_frames)
    resume_output = subprocess.run([exe, "--replay", str(resume_file)], capture_output=True, text=True, check=True).stdout
    resume_lines = [line for line in resume_output.splitlines() if line.startswith("observed frame:")]
    after_resume = next(line for line in resume_lines if int(line.split()[2]) > 44100 * 23 + 512)
    assert "tempo: unknown state: acquiring" in after_resume, after_resume
    assert "state: tracking" in resume_lines[-1], resume_lines[-1]
    missing = subprocess.run([exe, str(root / "missing.wav")], capture_output=True, text=True)
    assert missing.returncode != 0
    assert missing.stderr.startswith("error: "), missing.stderr
    import json
    quiet_json = json.loads(subprocess.run([exe, "--json", str(silence)], capture_output=True, text=True, check=True).stdout)
    assert quiet_json["schema_version"] == 1 and quiet_json["type"] == "result"
    assert quiet_json["bpm"] is None and quiet_json["state"] == "silent"
    result_json = json.loads(subprocess.run([exe, "--json", str(clicks)], capture_output=True, text=True, check=True).stdout)
    assert result_json["sample_rate"] == 44100 and result_json["frames"] == 44100 * 20
    assert result_json["duration_seconds"] == 20.0
    assert abs(result_json["bpm"] - bpm) < .01
    replay_json = [json.loads(line) for line in subprocess.run([exe, "--json", "--replay", str(clicks)], capture_output=True, text=True, check=True).stdout.splitlines()]
    assert replay_json[-1] == result_json
    beat_events = [item for item in replay_json if item.get("type") == "beat"]
    assert beat_events and all(item["frame"] < item["observed_through"] for item in beat_events)
    observations = [item for item in replay_json[:-1] if item.get("type") == "replay"]
    assert observations[0]["bpm"] is None and observations[-1]["observed_through"] == 44100 * 20
    prefix_json = [
        item for item in (
            json.loads(line)
            for line in subprocess.run(
                [exe, "--replay", "--json", str(prefix)],
                capture_output=True,
                text=True,
                check=True,
            ).stdout.splitlines()
        )
        if item.get("type") == "replay"
    ]
    # Metadata frames describes the whole input, so compare only causal fields.
    for earlier, full in zip(prefix_json, observations):
        for field in ("observed_through", "bpm", "beats", "state"):
            assert earlier[field] == full[field], (field, earlier, full)
    assert len(prefix_json) < len(observations)
    assert subprocess.run([exe, "--json", "--json", str(clicks)], capture_output=True).returncode == 2
    assert subprocess.run([exe, "--replay", "--replay", str(clicks)], capture_output=True).returncode == 2
    dash_path = root / "-clicks.wav"
    dash_path.write_bytes(clicks.read_bytes())
    dash_result = subprocess.run([exe, "--json", "--", "-clicks.wav"], cwd=root, capture_output=True, text=True, check=True)
    assert json.loads(dash_result.stdout) == result_json
