# Selected playback reopen

The Library playback controls expose **Reopen selected** for the currently
selected audio file. From stopped playback it opens the source; during active
or paused playback it replaces the source. Both paths allocate a newer
playback epoch and reset the cursor to frame zero, allowing stale audio and
visualization snapshots to be rejected by the existing epoch contract.
