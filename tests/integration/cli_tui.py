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
    first = subprocess.run(
        [exe, "--snapshot", str(root), "library"],
        capture_output=True, text=True, encoding="utf-8", errors="replace", env=cache_env,
    )
    assert first.returncode == 0, first.stderr
    second = subprocess.run(
        [exe, "--snapshot", str(root), "library"],
        capture_output=True, text=True, encoding="utf-8", errors="replace", env=cache_env,
    )
    assert second.returncode == 0, second.stderr
    second_plain = re.sub(r"\x1b\[[0-9;]*m", "", second.stdout)
    assert "persisted cache hits" in second_plain
    assert "[cached]" in second_plain

for tab, heading in (("library", "LIBRARY"), ("live", "LIVE"), ("settings", "SETTINGS")):
    result = subprocess.run(
        [exe, "--snapshot", str(root), tab],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
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
        assert "[analyzed]" in plain
    if tab == "live":
        for control in ("Reset / reacquire", "½ BPM", "Normal BPM", "2× BPM"):
            assert control in plain, (tab, control, result.stdout)

invalid = subprocess.run(
    [exe, "--snapshot", str(root), "not-a-tab"],
    capture_output=True,
    text=True,
    encoding="utf-8",
    errors="replace",
)
assert invalid.returncode == 2
assert "Invalid snapshot tab" in invalid.stderr
