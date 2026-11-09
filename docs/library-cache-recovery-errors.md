# Library cache lookup and invalidation errors

Cache lookup and invalidation run on the storage worker, but malformed or
incomplete identities can still be rejected by the storage owner. The desktop
bridge now catches those failures, clears a failed lookup's cached-result
presentation, and emits actionable `Cache lookup rejected:` or `Cache
invalidation rejected:` status text. A storage exception therefore cannot
escape a queued UI invocation or leave an earlier cache hit displayed as
current; failed invalidation also clears the stale presentation
conservatively. A new cache lookup likewise clears any prior admission
readout, so results from separate identity probes cannot be combined.

Successful invalidation continues to clear the cached result only after the
storage owner confirms removal. It also clears the displayed analysis and
admission state, making reanalysis an explicit next action. These paths do not
mutate source audio or reserve analysis work.

Admission responses synchronize the cached-result presentation, keeping the
bridge's cache-hit and admission properties consistent for one identity.
Lookup completion also clears stale retry/error state from a previous job, so
a fresh identity probe cannot inherit recovery controls from unrelated work.
Both transitions also reset stale progress so a completed lookup cannot appear
to be an active analysis job.
Lookup completion also publishes an explicit `Cached analysis ready` or `Cache
lookup complete` status rather than leaving the prior job status visible.
On a miss, any prior displayed result is cleared before fresh analysis can be
admitted, so an old result cannot be mistaken for the current source.
