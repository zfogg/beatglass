---
name: beatglass-documentation
description: Maintain Beatglass's MyST, Hawkmoth, and Sphinx documentation, including paginated C++ API pages, theme, coverage checks, and Pages publishing.
---

# Beatglass Documentation

Use this skill when adding or changing Beatglass documentation, C++ header contracts,
API reference entries, qualification notes, design notes, or the documentation build.

## Toolchain and published site

Beatglass uses three layers:

- MyST renders repository Markdown without converting the source files to RST.
- Hawkmoth extracts C++ declarations and comments from header/source files using
  `cpp:autodoc` filename patterns.
- Sphinx assembles, indexes, themes, and publishes both sources through GitHub Pages.

The published site uses a custom dark theme with pastel mint, lavender, and peach
accents, plus a styled green scrollbar. Keep that visual system when changing
`docs/api/_static/custom.css`; preserve readable contrast and narrow-screen wrapping.

## Source of truth

- Keep project, engineering, qualification, and design prose in Markdown under
  `docs/` or `design/`; the files are included in the Sphinx site through MyST.
- Keep C++ API and contract comments on the declarations they describe. New or
  changed headers under `include/`, `lib/`, and `src/` must be covered by a
  `cpp:autodoc` entry in the API pages. Prefer Hawkmoth filename globs for stable
  directories (for example `include/beatglass/library/detail/*.hpp`) instead of
  maintaining a long list of individual files. Hawkmoth uses glob patterns, not
  regular expressions.
- The API landing page is `docs/api/index.rst`; its page-level toctree should link
  to section pages such as `audio.rst`, `analysis.rst`, `desktop.rst`, and
  `experimental.rst` rather than accumulating every declaration on one page.
- Describe actual ownership, threading, timing, error behavior, and provisional
  status. Do not imply native qualification or API stability from documentation
  builds.

## Workflow

1. Update the relevant Markdown document or API section page. For C++ changes,
   add Hawkmoth-compatible comments to the declarations and extend the appropriate
   page's `cpp:autodoc` glob when a new directory pattern is needed.
2. If a new Markdown file should be published, add it to the project-documentation
   toctree in `docs/api/index.rst`; if it is an API section, add a dedicated RST
   page to the API toctree instead of extending the landing page.
3. Run the header coverage and rendered declaration checks:

   ```sh
   python docs/api/check_headers.py
   python tests/unit/check_headers_test.py
   ```

4. Build the pinned documentation image and run Sphinx with warnings as errors:

   ```sh
   docker build -f docs/api/Dockerfile -t beatglass-docs .
   docker run --rm -v "$PWD:/work" beatglass-docs \
     sphinx-build -W --keep-going -b html -c docs/api . /work/build/docs/html
   python docs/api/check_rendered.py build/docs/html/docs/api/index.html
   ```

On Windows PowerShell, use `${PWD}` for the Docker volume path. The site combines
MyST-rendered Markdown with Hawkmoth-generated C++ declarations. A successful build
checks documentation structure and declaration extraction; it does not qualify audio,
desktop UI, platform behavior, or release API stability.

## CI and publishing

`.github/workflows/docs.yml` publishes only on pushes to `main`. Do not add
pull-request or branch triggers without an explicit project decision. The workflow
runs header coverage, the strict Docker Sphinx build, and the rendered declaration
check before uploading and deploying the Pages artifact.
