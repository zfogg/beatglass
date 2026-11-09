# Playback stop shortcut

The playback stop shortcut now uses a guarded recovery helper that avoids
dispatching redundant stop requests after playback has already stopped.
