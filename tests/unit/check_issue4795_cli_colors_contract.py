"""Contract for CLI visualization color commands."""
from pathlib import Path

source = (Path(__file__).parents[2] / "src" / "cli" / "main.cpp").read_text(encoding="utf-8")
for marker in (
    "--show-colors",
    "--preview-colors",
    "--reset-colors",
    "invalid ",
    "expected #rrggbb",
    "color preview/reset cannot be combined",
    'type\\\":\\\"visualization_colors',
    "default_low",
):
    assert marker in source, marker

assert source.index("if (show_colors || reset_colors)") < source.index("if (path.empty())")
print("issue #4795 CLI color contract: PASS")
