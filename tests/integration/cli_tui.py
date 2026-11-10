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

invalid = subprocess.run(
    [exe, "--snapshot", str(root), "not-a-tab"],
    capture_output=True,
    text=True,
    encoding="utf-8",
    errors="replace",
)
assert invalid.returncode == 2
assert "Invalid snapshot tab" in invalid.stderr
