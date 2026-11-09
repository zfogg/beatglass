# Library stale-result lifecycle

An analysis result is valid only while its corresponding job is complete. The
desktop bridge now clears the displayed result when a job enters `Analysis
cancelled` or `Analysis failed`, while preserving the retryable lifecycle
state. This prevents a previous successful result from being mistaken for the
current source after cancellation or failure; a fresh retry must publish a new
result.
