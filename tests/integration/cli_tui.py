import pathlib
import re
import subprocess
import sys
import os
import tempfile

exe = sys.argv[1]
root = pathlib.Path(sys.argv[2])

with tempfile.TemporaryDirectory(prefix="beatglass-cli-cache-") as config_dir:
    cache_env = os.environ.copy()
    if os.name == "nt":
        cache_env["APPDATA"] = config_dir
    else:
        cache_env["XDG_CONFIG_HOME"] = config_dir
    settings_dir = pathlib.Path(config_dir) / ("Beatglass" if os.name == "nt" else "beatglass")
    settings_dir.mkdir(parents=True, exist_ok=True)
    (settings_dir / ("beatglass-cli.settings" if os.name == "nt" else "settings")).write_text(
        "reduce_motion 1\nseek_step 17\nvisual_mode beat_grid\nbpm_multiplier 1\n",
        encoding="utf-8",
    )
    first = subprocess.run(
        [exe, "--snapshot", str(root), "library"],
        capture_output=True, text=True, encoding="utf-8", errors="replace", env=cache_env,
    )
    assert first.returncode == 0, first.stderr
    if os.name == "nt":
        assert "\r\r\n" not in first.stdout, "snapshot output contains doubled carriage returns"
    second = subprocess.run(
        [exe, "--snapshot", str(root), "library"],
        capture_output=True, text=True, encoding="utf-8", errors="replace", env=cache_env,
    )
    assert second.returncode == 0, second.stderr
    second_plain = re.sub(r"\x1b\[[0-9;]*m", "", second.stdout)
    assert re.search(r"\d+ cache", second_plain)
    assert "[cached]" in second_plain

    for tab, heading in (("library", "LIBRARY"), ("live", "LIVE"), ("settings", "SETTINGS")):
        result = subprocess.run(
            [exe, "--snapshot", str(root), tab],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=cache_env,
        )
        assert result.returncode == 0, result.stderr
        plain = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout)
        assert heading in plain, (tab, result.stdout)
        for shortcut in ("[1] Library", "[2] Live", "[3] Settings"):
            assert shortcut in plain, (tab, shortcut, result.stdout)
        if tab == "library":
            assert "Analyze all" in plain
            assert "Analyze selected" in plain
            assert "Analyze all visible" in plain
            assert "Sort: title" in plain
            assert "Sort title/path" in plain
            assert "Source: " in plain
            assert "[analyzed]" in plain or "[cached]" in plain
            assert "WAVEFORM" in plain
            assert any(glyph in plain for glyph in ("▁", "▂", "▃", "▄", "▅", "▆", "▇", "█")), "snapshot lost colored waveform glyphs"
        if tab == "live":
            for control in ("Reset / reacquire", "½ BPM", "Normal BPM", "2× BPM"):
                assert control in plain, (tab, control, result.stdout)
            assert "Capture state: Stopped" in plain
            assert "Key: —    Chord: —" in plain
            assert "active 2×" in plain
        if tab == "settings":
            assert "Seek step (1–60 seconds): 17" in plain
            assert "Mode: beat_grid" in plain
            assert "Spinner animation paused" in plain

    invalid = subprocess.run(
        [exe, "--snapshot", str(root), "not-a-tab"],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        env=cache_env,
    )
    assert invalid.returncode == 2
    assert "Invalid snapshot tab" in invalid.stderr
