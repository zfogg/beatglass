import pathlib
import subprocess
import sys

exe = sys.argv[1]
root = pathlib.Path(sys.argv[2])

for tab, heading in (("library", "LIBRARY"), ("live", "LIVE"), ("settings", "SETTINGS")):
    result = subprocess.run(
        [exe, "--snapshot", str(root), tab],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    assert result.returncode == 0, result.stderr
    assert heading in result.stdout, (tab, result.stdout)
    for shortcut in ("[1] Library", "[2] Live", "[3] Settings"):
        assert shortcut in result.stdout, (tab, shortcut, result.stdout)
    if tab == "library":
        assert "Analyze all" in result.stdout
        assert "Analyze selected" in result.stdout
        assert "Analyze all visible" in result.stdout
        assert "Sort: title" in result.stdout
        assert "Sort title/path" in result.stdout
        assert "Source: " in result.stdout
        assert "[analyzed]" in result.stdout
    if tab == "live":
        for control in ("Reset / reacquire", "½ BPM", "Normal BPM", "2× BPM"):
            assert control in result.stdout, (tab, control, result.stdout)

invalid = subprocess.run(
    [exe, "--snapshot", str(root), "not-a-tab"],
    capture_output=True,
    text=True,
    encoding="utf-8",
    errors="replace",
)
assert invalid.returncode == 2
assert "Invalid snapshot tab" in invalid.stderr
