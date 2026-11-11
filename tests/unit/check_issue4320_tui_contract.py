"""Static contract checks for the initial issue #4320 TUI prototype."""
from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[2]
    tui = (root / "src" / "cli" / "tui_main.cpp").read_text(encoding="utf-8")
    required = {"FTXUI screen": "ScreenInteractive::TerminalOutput", "library surface": 'text("LIBRARY")', "analysis surface": 'text("TRACK ANALYSIS")', "now playing surface": 'text("NOW PLAYING")', "real analyzer": "beatglass::analyze_file", "audio discovery": "std::filesystem::directory_iterator", "keyboard navigation": "Event::ArrowDown", "analyze action": "Event::Return", "error recovery": 'status = std::string("Analysis failed: ")', "v2 navigation model": "NavigationModel", "command palette": "toggle_palette"}
    missing = [name for name, marker in required.items() if marker not in tui]
    if missing: raise SystemExit("Missing issue #4320 TUI markers: " + ", ".join(missing))
    print(f"Issue #4320 TUI contract markers verified ({len(required)} checks)")
    return 0

if __name__ == "__main__": raise SystemExit(main())
