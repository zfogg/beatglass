# Library analysis format gate

The desktop analysis bridge now inspects a selected file before creating a
worker operation. Missing files and metadata-only formats such as OGG/M4A are
rejected with a decoder-capability message before any queue admission or cache
reservation occurs. This keeps a metadata entry from being presented as a
successful analysis workflow.

The gate is deliberately separate from decoder qualification: WAV, FLAC, MP3,
and AIFF/AIF are currently marked analysis-capable by the file-details API, but
the configured libsndfile build still has to decode them successfully. AAC/M4A
remains unsupported until a decoder, licensing review, fixtures, and platform
matrix are qualified together.
