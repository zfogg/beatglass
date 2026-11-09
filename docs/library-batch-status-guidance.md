# Library batch status recovery guidance

Batch queue status now explains that rejected work reflects admission limits,
while terminal failures use Retry or discard recovery. This separates queue
capacity feedback from recoverable work failures for assistive technology.
