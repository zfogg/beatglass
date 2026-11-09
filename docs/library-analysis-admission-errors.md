# Library analysis admission errors

Admission failures now publish both the non-reserved empty admission result
and the stable `Analysis admission rejected` lifecycle status. The UI therefore
cannot remain on a prior queued or cached state when malformed identity or
storage errors prevent a new decision.
