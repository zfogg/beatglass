# Library batch discard stale-result reset

Confirming batch recovery discard now invalidates prior visualization evidence.
The retained recovery state is cleared without modifying source audio, and a new
accepted result is required before the visualization can be considered current.
