# Library cache reanalysis focus

The Run fresh analysis action opens a confirmation dialog because it invalidates
the selected cache identity before queueing work. The dialog announces that the
source audio remains unchanged, places initial focus on Cancel, and restores focus
to Run fresh analysis after either acceptance or cancellation. This keeps keyboard
users in the cache recovery workflow while making the destructive scope explicit.
