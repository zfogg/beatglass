# Play selected track action

The Library's Play selected control now uses a guarded action helper. It handles
missing selection or playback bridge state before dispatching, reports whether
the bridge accepted the source, and restores focus only after success.
