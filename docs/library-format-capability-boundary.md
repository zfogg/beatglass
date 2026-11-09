# Library format capability boundary

The Library can inspect metadata for a broader set of filename extensions than
the current analysis reader can decode. `FileDetails::analysis_supported` now
makes that distinction explicit: WAV, FLAC, MP3, and AIFF/AIF are currently
eligible for the file-reader analysis path, while OGG and M4A remain
metadata-only.

In particular, an M4A entry must not be treated as evidence that AAC/ALAC PCM
was decoded or that analysis can be queued. The file reader continues to
return `unsupported_format` for AAC/M4A. A future decoder change must update
the capability field, choose and license the decoder, add fixtures, and qualify
the supported platform matrix together.
