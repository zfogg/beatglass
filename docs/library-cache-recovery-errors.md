# Library cache lookup and invalidation errors

Cache lookup and invalidation run on the storage worker, but malformed or
incomplete identities can still be rejected by the storage owner. The desktop
bridge now catches those failures, clears a failed lookup's cached-result
presentation, and emits actionable `Cache lookup rejected:` or `Cache
invalidation rejected:` status text. A storage exception therefore cannot
escape a queued UI invocation or leave an earlier cache hit displayed as
current.

Successful invalidation continues to clear the cached result only after the
storage owner confirms removal. These paths do not mutate source audio or
reserve analysis work.
