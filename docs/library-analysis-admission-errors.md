# Library analysis admission errors

The cache/admission probe is a storage-owner boundary and can reject malformed
identities or unavailable storage. The desktop bridge now catches those
failures, emits an empty non-reserved admission result, and reports
`Analysis admission rejected:`. This prevents a failed probe from being
interpreted as either a cache hit or a queued analysis.
