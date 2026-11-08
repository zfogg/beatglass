# Beatglass documentation

This directory is the source of truth for project status, contracts, platform
qualification, and implementation decisions. The same Markdown files are
published in the Sphinx site through MyST, so links here work both in GitHub and
in the rendered documentation.

## Start here

- [Current progress and handoff](progress.md)
- [Phase 0 gate](phase0-gate.md)
- [Platform qualification status](platform-qualification-status.md)
- [Architecture contracts](contracts.md)
- [Recorded decisions](decisions.md)

## How the documentation is organized

### Status and coordination

These documents answer what is implemented, what is accepted, and what remains
open: [progress](progress.md), [phase 0 gate](phase0-gate.md), [foundation
handoff](foundation-handoff.md), and [platform status](platform-qualification-status.md).

### Architecture and contracts

These documents define boundaries and proposed interfaces: [contracts](contracts.md),
[phase 1 proposals](phase1-contract-proposals.md), [PipeWire status contract](pipewire-status-contract.md),
[collection UI integration](collection-ui-integration.md), [audio format support](audio-format-support.md),
[library duration contract](library-duration-contract.md), [dependency identity](dependency-identity-proposal.md),
[epoch handoff](epoch-handoff-proposal.md), and [same-source proposal](same-source-proposal.md).

### Platform and qualification

Use the platform-specific plans and evidence together with the [qualification
status](platform-qualification-status.md): [Linux plan](linux-qualification-plan.md),
[Linux increment](linux-platform-increment.md), [Linux evidence template](linux-native-evidence-template.md),
[macOS qualification](macos-qualification.md), [Core Audio status](macos-coreaudio-status.md),
[macOS overlay](macos-overlay.md), [Windows qualification](windows-qualification.md),
[Windows platform](windows-platform.md), [Windows overlay](windows-overlay.md), and
[packaging qualification](packaging-qualification.md).

### Research and experiments

Experimental work is intentionally separated from product commitments: [Essentia
spike](essentia-spike.md) and [tonal experiment](tonal-experiment.md).

## Publishing and API reference

The [API reference](api/index.rst) combines these Markdown documents with
Hawkmoth-generated C++ declarations. See the repository's
[`skills/beatglass-documentation`](../skills/beatglass-documentation/SKILL.md)
skill for the required build and validation commands.
