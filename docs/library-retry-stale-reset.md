# Library retry stale-result reset

Both worker-level and persisted-storage analysis retries now invalidate prior
visualization evidence immediately after retry admission. The selected source and
recovery controls remain available, but an earlier result cannot appear current
while replacement analysis is queued or running.
