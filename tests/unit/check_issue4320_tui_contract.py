"""Static contract checks for the initial issue #4320 TUI prototype."""
from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[2]
    tui = (root / "src" / "cli" / "cli_main.cpp").read_text(encoding="utf-8")
    tool = (root / "src" / "cli" / "tool_main.cpp").read_text(encoding="utf-8")
    required = {"FTXUI screen": "ScreenInteractive::TerminalOutput", "library surface": 'text("LIBRARY")', "analysis surface": 'text("TRACK ANALYSIS")', "live surface": 'text("LIVE")', "settings surface": 'text("SETTINGS")', "interactive settings": 'Checkbox("Reduce motion"', "seek setting": 'Input(&seek_step_text', "real analyzer": "beatglass::replay_file", "progress status": "analysis_frames", "async analysis": "std::async(std::launch::async", "analysis cancellation": "analysis_stop.request_stop", "waveform cancellation": "decode_waveform(path, stop_token)", "cancel action": 'Button(" Cancel analysis "', "custom completion event": "Event::Custom", "audio discovery": "std::filesystem::recursive_directory_iterator", "library filter": "Input(&filter", "folder control": 'Input(&folder_text', "folder action": 'Button(" Open folder "', "native folder picker": "FOS_PICKFOLDERS", "refresh sinks": 'Button(" Refresh sinks "', "stale sink validation": "select_render_endpoint", "live telemetry": 'live_observed_frames', "keyboard navigation": "MenuOption", "tab shortcuts": 'Event::Character("1")', "tab cycling": "Event::Tab", "mouse tabs": "Button(\"  Library  \"", "fullscreen layout": "| flex", "compact terminal": "needs more room", "open-folder shortcut": 'Event::Character("o")', "waveform chart": "waveform_chart", "waveform cursor": "playback_fraction", "spinner": "spinner(6", "analyze action": "Event::Return", "playback shortcut": 'Event::Character(" ")', "playback position": "current_frame()", "seek controls": 'seek_seconds', "cli version": '"beatglass-cli "', "cli help": 'Usage: beatglass-cli', "tool rename": "beatglass-tool", "error recovery": 'status = std::string("Analysis failed: ")'}
    missing = [name for name, marker in required.items() if marker not in (tool if name == "tool rename" else tui)]
    if missing: raise SystemExit("Missing issue #4320 TUI markers: " + ", ".join(missing))
    print(f"Issue #4320 TUI contract markers verified ({len(required)} checks)")
    return 0

if __name__ == "__main__": raise SystemExit(main())
