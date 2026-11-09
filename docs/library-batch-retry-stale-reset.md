# Library batch retry stale-result reset

Retrying a failed or cancelled analysis batch now invalidates previous
visualization evidence after retry admission. The batch remains visible and
focused while fresh accepted results are produced, so retained output cannot be
mistaken for the replacement analysis.
