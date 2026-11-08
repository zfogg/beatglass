#!/bin/bash
set -euo pipefail

# Transfer the exact local Git tree to a bounded temporary checkout on a Mac.
# This does not run a build or change the source repository.
remote_host="${1:-zachbook-pro}"
remote_dir="${2:-/tmp/beatglass-native-qualification}"
ref="${3:-HEAD}"

git rev-parse --verify "$ref" >/dev/null
printf 'Transferring %s to %s:%s\n' "$(git rev-parse "$ref")" "$remote_host" "$remote_dir"
git archive --format=tar "$ref" |
  ssh "$remote_host" "mkdir -p '$remote_dir' && tar -xf - -C '$remote_dir'"
ssh "$remote_host" "git -C '$remote_dir' rev-parse --is-inside-work-tree 2>/dev/null || true"
printf 'Remote checkout prepared at %s:%s\n' "$remote_host" "$remote_dir"
