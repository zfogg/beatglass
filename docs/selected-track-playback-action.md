# Selected-track playback action

The Library exposes a direct **Play selected** action next to the source-open
control. It opens the selected source through the playback bridge using a fresh
epoch, announces success or failure, and returns focus after a successful
request. The source file remains read-only; playback changes session state only.
