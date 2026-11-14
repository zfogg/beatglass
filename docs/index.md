# Beatglass documentation

Beatglass is a cross-platform music workspace for capture, analysis, library
management, playback, and live visualization. This site is the project’s
searchable documentation home: engineering notes, platform evidence, design
decisions, and the generated C++ API reference live together here.

```{toctree}
:maxdepth: 2
:caption: Start here

README
progress
platform-qualification-status
phase0-gate
foundation-handoff
```

## Find your way around

```{toctree}
:maxdepth: 2
:caption: Architecture and contracts

contracts
decisions
phase1-contract-proposals
audio-format-support
collection-ui-integration
library-catalog-contract
library-file-details-contract
library-fingerprint-contract
library-format-support
library-recovery-contract
library-storage-open-contract
library-storage-owner-contract
visualization-playback-contract
playback-visualization-frame-mapping
```

```{toctree}
:maxdepth: 2
:caption: Platform and qualification

linux-qualification-plan
linux-platform-increment
linux-native-evidence-template
macos-qualification
macos-coreaudio-status
macos-overlay
windows-qualification
windows-platform
windows-overlay
packaging-qualification
native-qualification-evidence
```

```{toctree}
:maxdepth: 2
:caption: API and design

api/index
../design/v1/README
../design/v1/PRODUCT
../design/v1/DESIGN
../design/v1/STATES
overlay-design-system
overlay-design-gallery
```

```{toctree}
:maxdepth: 2
:caption: Research and experiments

essentia-spike
tonal-experiment
issue-76-visual-implementation-audit
```

## Documentation map

The repository contains a large set of focused Markdown notes for interaction
contracts, accessibility behavior, recovery flows, and qualification evidence.
They remain in `docs/` as small, reviewable source files and are linked from
the relevant section pages above. The [API reference](api/index.rst) is the
best entry point when you need generated declarations alongside those notes.

## Contributing to the docs

Documentation is built with MyST, Sphinx, and Hawkmoth. See the
[`beatglass-documentation` skill](../skills/beatglass-documentation/SKILL.md)
for the local build, header coverage, and rendered-declaration checks.
