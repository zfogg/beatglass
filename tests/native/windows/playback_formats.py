"""Opt-in Windows playback/seek probe; records loopback only in memory.

Requires Python soundcard/numpy, FFmpeg, the matching Qt qmltestrunner and an
explicit render device. It generates test tones in the requested work directory;
it never opens user music or changes the system default. See docs/progress.md.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

import numpy as np
import soundcard as sc


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--qmltestrunner", required=True, type=Path)
    parser.add_argument("--device", required=True, help="Exact output device name")
    parser.add_argument("--work-dir", required=True, type=Path)
    parser.add_argument("--minimum-peak", type=float, default=1.0,
                        help="Uncalibrated FFT threshold; adjust for output level")
    args = parser.parse_args()
    args.work_dir.mkdir(parents=True, exist_ok=True)
    source = (Path(__file__).resolve().parents[3] / "src/desktop/qml").as_uri()
    speaker = sc.get_speaker(args.device)
    if speaker.name != args.device:
        raise RuntimeError(f"Device lookup returned {speaker.name!r}, not {args.device!r}")
    loopback = sc.get_microphone(speaker.id, include_loopback=True)
    if not loopback.isloopback:
        raise RuntimeError("Selected capture endpoint is not loopback")
    variants = [("wav", 523, ["-c:a", "pcm_s16le"]),
                ("flac", 659, ["-c:a", "flac"]),
                ("mp3", 784, ["-c:a", "libmp3lame", "-b:a", "128k"]),
                ("aiff", 988, ["-c:a", "pcm_s16be"])]
    env = os.environ.copy()
    env.pop("QT_MEDIA_BACKEND", None)
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["QT_QUICK_BACKEND"] = "software"
    for extension, frequency, codec in variants:
        folder = args.work_dir / extension
        folder.mkdir(exist_ok=True)
        target = folder / f"tone.{extension}"
        subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
                        "-f", "lavfi", "-i",
                        f"sine=frequency={frequency}:sample_rate=48000:duration=2",
                        "-ac", "1", *codec, str(target)], check=True)
        qml = '''import QtQuick
import QtTest
import QtMultimedia
import SOURCE
TestCase {
 name: "NativeFormat"; when: windowShown
 QtObject { id: meta; signal statusChanged(string status); function inspect(url) {} }
 QtObject { id: guard; function diagnostic(device) { return "" } }
 QtObject { id: live; property string status: "Stopped"; property string tempo: "—"; property string key: "—"; property string chord: "—" }
 Component { id: c; Main { visible: true; metadataBridge: meta; playbackGuardBridge: guard; liveBridge: live } }
 function test_playSeek() {
  let a = c.createObject(null); let s = findChild(a, "playbackOutputSelector"); let selected = -1
  tryVerify(() => { for (let i = 0; i < s.count; ++i) if (s.model[i].description === DEVICE) { selected = i; return true } return false }, 10000)
  s.currentIndex = selected; s.activated(selected); a.browseFolder = FOLDER
  let m = findChild(a, "fileModel"); tryVerify(() => m.count === 1, 10000); a.activateEntry(0)
  let p = findChild(a, "player"); tryVerify(() => p.duration >= 1800 && p.seekable, 10000)
  findChild(a, "playButton").clicked(); tryCompare(p, "playbackState", MediaPlayer.PlayingState, 10000)
  tryVerify(() => p.position > 200, 5000); p.position = 1000; tryVerify(() => p.position >= 900, 5000)
  wait(500); p.stop(); a.destroy()
 }
}
'''.replace("import SOURCE", "import " + json.dumps(source)).replace("DEVICE", json.dumps(args.device)).replace("FOLDER", json.dumps(folder.resolve().as_uri()))
        (folder / "tst_native.qml").write_text(qml, encoding="utf-8")
        result = folder / "result.txt"
        with loopback.recorder(samplerate=48000, channels=2) as recorder:
            process = subprocess.Popen([str(args.qmltestrunner), "-input", str(folder),
                                        "-o", str(result) + ",txt"], env=env,
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            try:
                pcm = recorder.record(numframes=48000 * 15)
                code = process.wait(timeout=30)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
        mono = pcm.mean(axis=1)
        block_size = 4800
        blocks = mono[:len(mono) // block_size * block_size].reshape(-1, block_size)
        spectrum = np.abs(np.fft.rfft(blocks * np.hanning(block_size), axis=1))
        frequencies = np.fft.rfftfreq(block_size, 1 / 48000)
        index = int(np.argmin(abs(frequencies - frequency)))
        peak = float(np.max(spectrum[:, max(0, index - 1):index + 2]))
        print(f"{extension}: QtTest exit={code}, {frequency} Hz peak={peak:.3f}", flush=True)
        if code or peak < args.minimum_peak:
            raise RuntimeError(f"{extension} playback probe failed; see {result}")


if __name__ == "__main__":
    main()
