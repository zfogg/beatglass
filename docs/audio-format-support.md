# Audio format support status

Beatglass's current catalog contract includes regular files identified as WAV,
FLAC, MP3, and AIFF. The catalog records the source path and format label, but
format discovery is not a promise that every build can decode every codec:
the actual libsndfile build and its enabled features determine runtime support.

The v1 target formats are qualified only when the decoder integration test
passes in the exact build. A development machine whose libsndfile lacks an
optional codec may disable that integration test, but that does not establish
format qualification. Unsupported input must report a decoder error or unknown
result rather than pretending that analysis ran.

AAC/M4A is not currently a supported v1 format. Its decoder and distribution
licensing have not been selected or qualified. Do not add an AAC/M4A extension
to the product support list until a concrete decoder, platform matrix, license
review, and regression fixture are documented.

Seeing a filename or catalog entry is not evidence that PCM was decoded or
that analysis ran.
