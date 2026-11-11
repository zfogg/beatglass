"""Static contract for the CLI popup and cancellation slice."""
from pathlib import Path

source = (Path(__file__).parents[2] / "src" / "cli" / "tui_main.cpp").read_text(encoding="utf-8")
for marker in (
    "enum class Popup",
    'Popup::folder',
    'Popup::output',
    'Popup::colors',
    'Popup::settings',
    'Popup::permissions',
    'Popup::recovery',
    'Popup::confirm_remove',
    'Escape always cancels this popup',
    'if (event == Event::Escape) { popup = Popup::none',
    'User audio files will not be deleted.',
    'Folder is unavailable; choose an existing directory.',
    'reload_folder(chosen);',
    'Library folder removed. User audio files were not deleted.',
    'Event::Backspace',
):
    assert marker in source, marker

# Escape must be handled by the modal branch before the application quit branch.
assert source.index("if (popup != Popup::none)") < source.index("if (event == Event::Escape) { screen.Exit();")
print("issue #4801 TUI popup contract: PASS")
