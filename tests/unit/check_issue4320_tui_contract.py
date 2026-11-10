"""Static contract checks for the initial issue #4320 TUI prototype."""
from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[2]
    tui = (root / "src" / "cli" / "cli_main.cpp").read_text(encoding="utf-8")
    tool = (root / "src" / "cli" / "tool_main.cpp").read_text(encoding="utf-8")
    required = {"FTXUI screen": "ScreenInteractive::TerminalOutput", "library surface": 'text("LIBRARY")', "analysis surface": 'text("TRACK ANALYSIS")', 'source path': 'text("Source: "', 'single analysis action': 'Button(" Analyze selected "', 'batch action': 'Button(" Analyze all "', 'batch progress': 'batch_completed', 'batch error isolation': 'analysis_errors', 'retry clears error': 'analysis_errors.erase', 'row analyzed badge': '[analyzed]', 'row failed badge': '[failed]', 'sort action': 'Sort: title', 'sort identity': 'Sorted visible Library by ', "live surface": 'text("LIVE")', 'live bpm correction': 'Button(" ½ BPM "', 'live reset': 'Button(" Reset / reacquire "', 'live correction state': 'live_bpm_multiplier', "settings surface": 'text("SETTINGS")', "interactive settings": 'Checkbox("Reduce motion"', "seek setting": 'Input(&seek_step_text', "real analyzer": "beatglass::replay_file", 'optional tonal analyzer': 'replay_music_file', 'tonal display': 'text("Key: "', 'tonal history': 'text("Tonal history: "', "progress status": "analysis_frames", "async analysis": "std::async(std::launch::async", "analysis cancellation": "analysis_stop.request_stop", "refresh cancellation": "Cancelling the previous analysis", "selection identity": "selection preserved by path", "selection cancellation": "Cancelling analysis because the selected track changed", "analysis reentry guard": "Analysis already in progress", "stale analysis guard": "Analysis finished for a previous track", "no track playback feedback": "Select a supported audio file before starting playback", "portable playback status": "Native playback is unavailable in this build", "waveform cancellation": "decode_waveform(path, stop_token)", "waveform legend": 'text("PCM amplitude")', "real beat positions": "beat_frames", "cancel action": 'Button(" Cancel analysis "', "custom completion event": "Event::Custom", "audio discovery": "std::filesystem::recursive_directory_iterator", "library filter": "Input(&filter", "folder control": 'Input(&folder_text', "folder action": 'Button(" Open folder "', "native folder picker": "FOS_PICKFOLDERS", "refresh sinks": 'Button(" Refresh sinks "', "live selection lifecycle": "Capture stopped because the selected sink changed", "live refresh lifecycle": "Capture stopped while render sinks were refreshed", "stale sink validation": "select_render_endpoint", "live telemetry": 'live_observed_frames', "live button state sync": 'live_button_label = " Start monitoring "', "pipewire backend": "BEATGLASS_HAS_PIPEWIRE", "pipewire discovery": "discover_sinks", "pipewire stream": "pop_current", "pipewire reacquisition": "requires_reacquisition", "shared live presentation": "beatglass::present_snapshot", "keyboard navigation": "MenuOption", "tab shortcuts": 'Event::Character("1")', "tab cycling": "Event::Tab", "mouse tabs": "Button(\"  [1] Library  \"", "visible tab shortcuts": "Button(\"  [3] Settings  \"", "tonal capability status": "Tonal analysis: unavailable", "fullscreen layout": "| flex", "compact terminal": "needs more room", "open-folder shortcut": 'Event::Character("o")', "waveform chart": "waveform_chart", "waveform cursor": "playback_fraction", "spinner": "spinner(6", "analyze action": "Event::Return", "playback shortcut": 'Event::Character(" ")', "playback position": "current_frame()", "seek controls": 'seek_seconds', "cli version": '"beatglass-cli "', "cli help": 'Usage: beatglass-cli', "snapshot tab validation": "Invalid snapshot tab", "tool rename": "beatglass-tool", "error recovery": 'status = std::string("Analysis failed: ")'}
    missing = [name for name, marker in required.items() if marker not in (tool if name == "tool rename" else tui)]
    if missing: raise SystemExit("Missing issue #4320 TUI markers: " + ", ".join(missing))
    if "Analysis delay: " not in tui: raise SystemExit("CLI Live view does not expose shared analysis delay")
    if "stale evidence" not in tui: raise SystemExit("CLI Live confidence must suppress stale evidence")
    if "live_presentation.stale || !live_bpm" not in tui: raise SystemExit("CLI Live tempo must suppress stale evidence")
    if "[cached]" not in tui: raise SystemExit("CLI Library does not distinguish persisted cache hits")
    if "std::vector<float> decoded(4096), render(4096)" not in tui:
        raise SystemExit("CLI playback buffers exceed the shared bounded session capacity")
    if "no fresh packets arrived; reacquiring" not in tui:
        raise SystemExit("CLI Live watchdog does not expose no-packet reacquisition")
    if "library_folder" not in tui or "std::quoted(preferences.library_folder)" not in tui:
        raise SystemExit("CLI Library folder preference is not persisted as a quoted path")
    if "!explicit_root && !preferences.library_folder.empty()" not in tui:
        raise SystemExit("CLI Library folder preference is not restored on startup")
    if "live_spectrum_history" not in tui or "FFT history" not in tui:
        raise SystemExit("CLI Live spectrum does not expose bounded FFT history")
    if '"source changed") != std::string::npos ? "  [stale]"' not in tui:
        raise SystemExit("CLI Library does not distinguish stale source analysis from failed analysis")
    if 'format_bpm(analyzed->second.result.bpm) + " BPM"' not in tui or '"  key " + *analyzed->second.key' not in tui:
        raise SystemExit("CLI Library rows do not expose analyzed BPM and key summaries")
    if "library_cache_hits += analysis.cache_hits" not in tui:
        raise SystemExit("CLI snapshot path does not expose persisted cache hits")
    if 'live_status = std::string("Displayed tempo correction set' not in tui or 'persist_preferences();' not in tui[tui.find('auto set_live_multiplier'):tui.find('auto half_bpm_button')]:
        raise SystemExit("CLI BPM interpretation setting is not persisted when changed")
    if 'Button(" Recover persisted "' not in tui:
        raise SystemExit("CLI Library does not expose persisted recovery action")
    for marker in ("live_endpoint_id", "live_endpoint_name", "live_endpoint_preference", "updated.live_endpoint_id", "updated.live_endpoint_name"):
        if marker not in tui:
            raise SystemExit("CLI Live endpoint preference contract missing: " + marker)
    print(f"Issue #4320 TUI contract markers verified ({len(required)} checks)")
    return 0

if __name__ == "__main__": raise SystemExit(main())
