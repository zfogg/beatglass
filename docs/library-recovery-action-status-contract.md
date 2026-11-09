# Library recovery action status

The Library exposes a read-only status beside persisted recovery controls. It
states whether retry is available, whether storage must be reopened, and whether
the selected run is still active. Retry and discard remain explicit actions and
never modify the original audio source.
