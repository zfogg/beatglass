Beatglass API reference
=======================

These interfaces describe the current foundation and are provisional. Headers
under ``include/beatglass`` are library interfaces, not a promise of a stable
installed API. Private ``src/`` and ``lib/`` headers are indexed for engineering
reference only; documenting them does not install or publish them.

The API is split into section pages so each area can be read and linked without
one extremely long document.

.. toctree::
   :maxdepth: 1
   :caption: C++ API sections

   audio
   storage
   analysis
   library-jobs
   desktop
   experimental
   cli

Repository documentation
------------------------

The Markdown documents below are included directly through MyST. They remain
Markdown source files for repository and GitHub use, while Sphinx provides one
searchable, cross-linked documentation site alongside the Hawkmoth C++ API.

.. toctree::
   :maxdepth: 2
   :caption: Navigation

   ../README.md
   ../../README.md

.. toctree::
   :maxdepth: 1
   :caption: Status and coordination

   ../progress.md
   ../phase0-gate.md
   ../foundation-handoff.md
   ../platform-qualification-status.md

.. toctree::
   :maxdepth: 1
   :caption: Architecture and contracts

   ../contracts.md
   ../decisions.md
   ../phase1-contract-proposals.md
   ../audio-format-support.md
   ../collection-ui-integration.md
   ../pipewire-status-contract.md
   ../library-duration-contract.md
   ../dependency-identity-proposal.md
   ../epoch-handoff-proposal.md
   ../same-source-proposal.md
   ../library-catalog-contract.md
   ../library-file-details-contract.md
   ../library-fingerprint-contract.md
   ../library-format-support.md
   ../library-recovery-contract.md
   ../library-storage-open-contract.md
   ../library-storage-owner-contract.md
   ../visualization-playback-contract.md
   ../playback-visualization-frame-mapping.md

.. toctree::
   :maxdepth: 1
   :caption: Platform and qualification

   ../linux-qualification-plan.md
   ../linux-platform-increment.md
   ../linux-native-evidence-template.md
   ../macos-qualification.md
   ../macos-coreaudio-status.md
   ../macos-overlay.md
   ../windows-qualification.md
   ../windows-platform.md
   ../windows-overlay.md
   ../packaging-qualification.md
   ../native-qualification-evidence.md

.. toctree::
   :maxdepth: 1
   :caption: Design and experiments

   ../../design/v1/README.md
   ../../design/v1/PRODUCT.md
   ../../design/v1/DESIGN.md
   ../../design/v1/STATES.md
   ../../design/v1/REVIEW.md
   ../essentia-spike.md
   ../tonal-experiment.md
   ../issue-76-visual-implementation-audit.md

.. toctree::
   :maxdepth: 1
   :caption: Contributor guidance

   ../../AGENTS.md
   ../../skills/beatglass-documentation/SKILL.md
   ../../tests/standalone/README.md
