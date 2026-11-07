# Screen and state contract

| Screen | Job and state | Required implementation behavior |
|---|---|---|
| Library | Browse indexed collection; completed, analyzing and source changed rows; track key unknown | Folder navigation, search, sort, select, individual/batch analysis; preserve files. |
| Library empty | First run before folder selection | Offer folder picker and explain source preservation and supported formats accurately. |
| Track | File-attributed detail with unknown track key, separately timestamped provisional tonal window, alternatives, tempo and chord timeline | Play/seek synchronized to results; clear prior evidence on selection change. |
| Jobs | Running progress, completed and changed source | Cancel current or remaining jobs, retry and explain changed source; prioritize live work. |
| DJ | Monitoring selected output, activity and combined-mix caveat | Start/stop, select default or explicit output, half/double BPM, reset/reacquire. |
| DJ recovery | Disconnected source with estimates cleared | Choose source, show unavailable state and reacquire only when capture resumes. |
| Overlay | Preview and settings, including click-through | Position, size, opacity, monitor, unlock/hide and persistent settings. |
| Overlay context | Floating panel over an illustrative DJ app | Keep overlay readable and recover control via platform-supported taskbar/tray or normal window; validate actual platform behavior. |
| Settings | Six themes and overlay preferences | Persist valid selections, support reduced motion and system scaling. |
| Recovery | No output, permission denied, missing file, changed source | Provide correct platform-specific next steps and keep stale evidence cleared. |

Additional implementation states to exercise: empty search; no selected track; unsupported/corrupt/inaccessible/moved files; partial/cancelled/failed analysis; unavailable playback device; silence, ambiguous mixed tracks, capture permission revoked, default-device switch and discontinuity; overlay hidden, unlocked, off-screen, monitor removed and compositor refusal. These are specified here but are not all separately rendered. Remove entry must explicitly say the audio file is retained. No fabricated confidence percentages or deck separation.

