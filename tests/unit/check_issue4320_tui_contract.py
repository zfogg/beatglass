"""Static contract checks for the initial issue #4320 TUI prototype."""
from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[2]
    tui = (root / "src" / "cli" / "cli_main.cpp").read_text(encoding="utf-8")
    tool = (root / "src" / "cli" / "tool_main.cpp").read_text(encoding="utf-8")
    required = {"FTXUI screen": "ScreenInteractive::TerminalOutput", "library surface": 'text("LIBRARY")', "analysis surface": 'text("TRACK ANALYSIS")', "live surface": 'text("LIVE")', "settings surface": 'text("SETTINGS")', "interactive settings": 'Checkbox("Reduce motion"', "real analyzer": "beatglass::analyze_file", "async analysis": "std::async(std::launch::async", "custom completion event": "Event::Custom", "audio discovery": "std::filesystem::recursive_directory_iterator", "library filter": "Input(&filter", "native endpoints": "enumerate_render_endpoints", "keyboard navigation": "Event::ArrowDown", "mouse tabs": "Button(\"  Library  \"", "waveform chart": "waveform_chart", "spinner": "spinner(6", "analyze action": "Event::Return", "tool rename": "beatglass-tool", "error recovery": 'status = std::string("Analysis failed: ")'}
    missing = [name for name, marker in required.items() if marker not in (tool if name == "tool rename" else tui)]
    if missing: raise SystemExit("Missing issue #4320 TUI markers: " + ", ".join(missing))
    print(f"Issue #4320 TUI contract markers verified ({len(required)} checks)")
    return 0

if __name__ == "__main__": raise SystemExit(main())
