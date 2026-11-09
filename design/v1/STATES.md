# Screen and state contract

| Screen | Job and state | Required implementation behavior |
|---|---|---|
| Library | Browse indexed collection; completed, analyzing and source changed rows; track key unknown | Folder navigation, search, sort, select, individual/batch analysis; preserve files. |
| Library search results | Search for “night” returns three source-attributed matches with mixed analysis states | Keep query/sort context visible, preserve verified source paths, and make actions target the selected file identity. |
| Track playback queue | Selected Night Drive source is verified, ready to play, and queued as 1 of 1 | Bind playback and seeking to the verified source, expose queue and accepted-evidence status, and fail closed if the file changes. |
| Track playback recovery | Queued Night Drive becomes unavailable before playback resumes | Stop playback, preserve source-attributed evidence, and offer locate/retry or explicit queue removal without substituting another file. |
| Track playback retrying | Located source is being checked before queue restoration | Hold the queue, hide evidence during identity verification, and keep cancellation/removal explicit while the source remains untouched. |
| Track playback restored | Located source matches the queued identity and playback is available again | Restore the queue, expose accepted evidence only after identity checks, and restart playback from a deterministic position. |
| Track playback canceled | Recovery is canceled after the queued source remains unavailable | Clear pending playback safely, preserve source history, and offer an explicit later retry without deleting or substituting files. |
| Track source details | Browse selection opens a verified Night Drive detail view with accepted and provisional evidence separated | Keep source identity visible, expose safe playback/reanalysis actions, and never fabricate unavailable track-level values. |
| Track reanalysis failed | A verified Night Drive reanalysis fails before publishing a result | Keep stale evidence hidden, preserve source identity, and offer explicit retry/discard recovery without modifying the audio file. |
| Track reanalysis discarded | Failed reanalysis is discarded without restoring an unaccepted result | Clear local run state, retain the verified source, and leave retry, queue, and browse actions explicit. |
| Track reanalysis ready | Verified source has no active run and is ready for a deliberate track-only retry | Show exact run scope, keep evidence clear, and publish only if source identity still matches at completion. |
| Track reanalysis running | Verified Night Drive track-only reanalysis is 68% complete | Keep stale evidence hidden, expose progress and cancel semantics, and hold playback/queue actions until identity-checked completion. |
| Track reanalysis canceled | Active verified-source reanalysis is canceled before publishing | Keep evidence clear, preserve the source, and offer explicit retry or queue actions without modifying audio. |
| Track reanalysis rejected | Completed run fingerprint no longer matches the verified source | Fail closed, keep the result hidden, preserve expected/observed provenance, and require locate/retry or discard. |
| Track source located | A possible replacement path is found and awaits explicit identity confirmation | Compare expected and observed paths, require fingerprint confirmation, and keep evidence/playback unavailable until confirmed. |
| Track source confirmed | Located file matches the expected identity after a path change | Restore provenance and playback eligibility, permit deliberate reanalysis, and fail closed on future changes. |
| Track reanalysis after move | Confirmed moved source completes a new accepted analysis | Link moved path to original catalog identity before restoring evidence/playback, preserving provenance and no-tag guarantees. |
| Track playback after move | Accepted recovered source is ready for playback and queue use | Bind playback to the confirmed recovered path, preserve provenance, and fail closed on future fingerprint mismatch. |
| Track playback move mismatch | Confirmed recovered source changes while playback is active | Stop playback immediately, hide stale evidence, lock queue actions, and require explicit source recovery. |
| Track playback move checking | Recovered source is being verified after a playback mismatch | Hold queue/playback, hide evidence, and keep cancellation/removal explicit until identity verification completes. |
| Track playback move confirmed | Recovered source passes identity verification after mismatch | Restore queue/playback and evidence eligibility only for the confirmed path, with future mismatch returning to recovery. |
| Track playback move canceled | Recovery is canceled before a recovered source is confirmed | Keep playback/evidence locked, clear active queue work, and preserve explicit retry/removal without touching files. |
| Track playback move removed | Unconfirmed recovered source is removed from the playback queue | Clear playback state while retaining source attribution and fail-closed evidence for later Browse recovery. |
| Track recovery later | Catalog entry remains after queue removal while source recovery is deferred | Preserve provenance, keep playback/evidence inactive, and make locate/remove catalog actions explicit. |
| Library folder navigation | User opens a nested catalog folder | Show breadcrumbs and folder-scoped actions while preserving source identity and the complete catalog. |
| Library folder empty | Indexed folder has no supported audio sources | Explain the folder-scoped empty state, offer navigation/add-folder actions, and preserve other catalog entries. |
| Library folder permission recovery | Indexed folder cannot be read by the operating system | Explain access denial, avoid starting jobs, and offer permissions/retry/alternate-folder recovery. |
| Library folder ready | Folder access was restored and sources rechecked | Summarize recovered identities, defer analysis until explicit action, and preserve files. |
| Library multi-selection | Several eligible tracks are selected for a batch action | Show selection count, eligible batch action, and preserve completed evidence and source files. |
| Library batch queued | Selected sources were admitted to analysis | Show admitted/skipped counts, link to queue, and preserve existing evidence until fresh results are accepted. |
| Library batch complete | Batch finished with accepted and attention-needed sources | Summarize accepted/review/skipped counts and route to results or recovery without losing attribution. |
| Library batch attention | Source changed before a fresh result could be accepted | Withhold stale output, explain the identity change, and route to verification/retry without mutating files. |
| Library batch cancellation | User requests cancellation while queued/active work remains | Confirm pending/active/completed counts and stop provisional work without removing accepted results or source files. |
| Library batch canceled | Pending and active batch work stopped | Summarize canceled/preserved counts, offer queue/browse paths, and preserve files and accepted evidence. |
| Library batch retry | Canceled/recoverable sources are ready for explicit retry | Show per-source recovery status, verify identity before admission, and publish no evidence until acceptance. |
| Library batch retry complete | Retry finished with accepted and recoverable outcomes | Summarize accepted/review counts, preserve source attribution, and never restore stale evidence. |
| Library import in progress | Folder scan is discovering and checking sources | Show bounded progress, preserve the last catalog snapshot, and make cancellation non-destructive. |
| Library import complete | Folder scan finished with new, unchanged, and review-needed sources | Summarize identity reuse, offer analysis for new entries, and preserve source files. |
| Library import review | Scan found unsupported, missing, or unreadable sources | Keep entries recoverable, provide locate/retry/remove actions, and never delete source files implicitly. |
| Library empty | First run before folder selection | Offer folder picker and explain source preservation and supported formats accurately. |
| Library filter empty | Search returns no matches | Explain the query, offer Clear search, and reassure that source files are unchanged. |
| Library filtered results | Active status/folder filters match catalog entries | Show removable filter chips, filtered count, scoped actions, and preserve the complete catalog. |
| Library saved filter view | User reopens a locally saved filter combination | Show saved-view status, edit/clear actions, and explain that preferences never mutate source audio. |
| Library filter builder | User edits rules for a saved view | Show explicit rules, preview count, save/cancel actions, and keep filtering non-destructive. |
| Library saved-view deletion | User removes a saved filter preference | Confirm preference-only impact, preserve matching tracks/evidence, and offer cancel/delete actions. |
| Library storage recovery | Saved folder exists but local storage is unavailable | Explain retained source files, offer reopen/choose/forget actions, and keep analysis disabled. |
| Jobs empty | No queued, running, or recoverable analysis | Explain that results remain available and offer a route back to Library. |
| DJ permission recovery | OS capture permission unavailable | Explain why estimates are blank, link to system settings, and preserve Library evidence. |
| DJ monitoring starting | Output capture is opening and reacquiring timing | Gate estimates until stable frames arrive, show bounded startup progress, and preserve Library evidence on cancel. |
| Overlay recovery | Overlay remains visible while its output source disconnects | Show no new estimates, preserve Library evidence, and provide a visible restore-controls path for source recovery. |
| Audio settings readiness | User configures a capture output before monitoring | Show permission/device readiness, provide a non-publishing test, and explain disconnect safety. |
| Track metadata loading | Selected source is being decoded | Show a non-fabricated loading state, keep prior evidence hidden, and provide cancellation. |
| Track metadata error | Decoder cannot read selected source | Explain cleared evidence, offer retry/another track, and state that files are unchanged. |
| Track analysis queued | Verified source admitted to analysis queue | Hide prior evidence, show worker status and cancellation, and link to queue. |
| Track analysis in progress | Active worker is processing the selected source | Show stage/progress, keep stale evidence hidden, and expose cancellation without mutating the audio file. |
| Track analysis canceled | Worker stopped before publishing | Explain cleared evidence, offer retry/discard recovery, and state that audio is unchanged. |
| Track analysis failed | Worker reported an error | Show actionable guidance, retry/discard recovery, and no stale visualization. |
| Track reanalysis confirmation | User requests fresh analysis for a verified source | Explain provisional evidence hiding, preserve the audio file, and offer keep/start actions. |
| Track reanalysis complete | Fresh analysis passed identity checks and was accepted | Show acceptance and updated metrics, preserve source attribution, and distinguish track-level versus provisional evidence. |
| Analysis queue paused | User paused the worker queue | Preserve accepted progress and completed results, expose resume/cancel actions, and never mutate source audio. |
| Analysis queue retrying | Recoverable worker or source error is being retried | Show retry progress, preserve completed evidence, and replace it only after a fresh run is accepted for the same source identity. |
| DJ paused | User paused capture intentionally | Show no new estimates, preserve existing Library evidence, and offer resume/reacquire. |
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
