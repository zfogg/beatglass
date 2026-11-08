---
name: beatglass-documentation
description: Document Beatglass C++ APIs and project Markdown with the repository's Hawkmoth and Sphinx workflow.
---

# Beatglass Documentation

Use this skill when adding or changing Beatglass documentation, C++ header contracts,
API reference entries, qualification notes, design notes, or the documentation build.

## Source of truth

- Keep project, engineering, qualification, and design prose in Markdown under
  `docs/` or `design/`; the files are included in the Sphinx site through MyST.
- Keep C++ API and contract comments on the declarations they describe. New or
  changed headers under `include/`, `lib/`, and `src/` must be represented in
  `docs/api/index.rst`.
- Describe actual ownership, threading, timing, error behavior, and provisional
  status. Do not imply native qualification or API stability from documentation
  builds.

## Workflow

1. Update the relevant Markdown document and, for C++ changes, add Hawkmoth-compatible
   comments to the declarations and an `.. cpp:autodoc::` entry if needed.
2. If a new Markdown file should be published, add it to the project-documentation
   `toctree` in `docs/api/index.rst`.
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
   python docs/api/check_rendered.py build/docs/html/index.html
   ```

On Windows PowerShell, use `${PWD}` for the Docker volume path. The site combines
MyST-rendered Markdown with Hawkmoth-generated C++ declarations. A successful build
checks documentation structure and declaration extraction; it does not qualify audio,
desktop UI, platform behavior, or release API stability.
