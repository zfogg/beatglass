# Screen and state contract

| Screen | Job and state | Required implementation behavior |
|---|---|---|
| Library | Browse indexed collection; completed, analyzing and source changed rows; track key unknown | Folder navigation, search, sort, select, individual/batch analysis; preserve files. |
| Library empty | First run before folder selection | Offer folder picker and explain source preservation and supported formats accurately. |
| Library filter empty | Search returns no matches | Explain the query, offer Clear search, and reassure that source files are unchanged. |
| Library storage recovery | Saved folder exists but local storage is unavailable | Explain retained source files, offer reopen/choose/forget actions, and keep analysis disabled. |
| Jobs empty | No queued, running, or recoverable analysis | Explain that results remain available and offer a route back to Library. |
| DJ permission recovery | OS capture permission unavailable | Explain why estimates are blank, link to system settings, and preserve Library evidence. |
| Track metadata loading | Selected source is being decoded | Show a non-fabricated loading state, keep prior evidence hidden, and provide cancellation. |
| Track | File-attributed detail with unknown track key, separately timestamped provisional tonal window, alternatives, tempo and chord timeline | Play/seek synchronized to results; clear prior evidence on selection change. |
| Jobs | Running progress, completed and changed source | Cancel current or remaining jobs, retry and explain changed source; prioritize live work. |
| DJ | Monitoring selected output, activity and combined-mix caveat | Start/stop, select default or explicit output, half/double BPM, reset/reacquire. |
| DJ recovery | Disconnected source with estimates cleared | Choose source, show unavailable state and reacquire only when capture resumes. |
| Overlay | Preview and settings, including click-through | Position, size, opacity, monitor, unlock/hide and persistent settings. |
| Overlay context | Floating panel over an illustrative DJ app | Keep overlay readable and recover control via platform-supported taskbar/tray or normal window; validate actual platform behavior. |
| Settings | Six themes and overlay preferences | Persist valid selections, support reduced motion and system scaling. |
| Recovery | Library file alert and analysis-stopped toast; device and permissions in Settings | Provide correct platform-specific next steps, notification fallback, and keep stale evidence cleared. |

Additional implementation states to exercise: empty search; no selected track; unsupported/corrupt/inaccessible/moved files; partial/cancelled/failed analysis; unavailable playback device; silence, ambiguous mixed tracks, capture permission revoked, default-device switch and discontinuity; overlay hidden, unlocked, off-screen, monitor removed and compositor refusal. These are specified here but are not all separately rendered. Remove entry must explicitly say the audio file is retained. No fabricated confidence percentages or deck separation.


## Hidden-window and recovery transition

1. Show overlay → monitoring continues and main window hides. Provide a Windows/Linux tray icon or macOS menu-bar icon; omit the main window from the taskbar while hidden where supported.
2. Tray/menu-bar actions restore controls, unlock/edit or hide the overlay, and quit. A second launch restores the existing window. No restore control is required on the overlay itself.
3. If the platform cannot expose the icon, retain a visible window or taskbar recovery route and explain why. Test icon disappearance, process relaunch, permission loss, display changes, shutdown and compositor refusal on actual platforms.

The overlay's default track-metadata state is unavailable: combined system output alone cannot identify a DJ app's tracks or decks. The underlying DJ application in the scene owns its own illustrative data. Any future DJ-app integration is optional and software-specific, must show provenance and freshness and clear on disconnect or deck change, and must not attribute combined-mix analysis to a deck without evidence. Generic MIDI is not a guaranteed live title/artist feed.
