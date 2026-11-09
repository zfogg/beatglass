# Playback device selection

Playback output labels use the native device identifier followed by a human
name. Selection parses and preserves that identifier instead of treating the
ComboBox row number as the device id, so refreshed or sparse device lists keep
the correct output target.
