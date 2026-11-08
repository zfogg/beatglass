#!/bin/bash
set -euo pipefail

# Transfer the exact local Git tree and revision metadata to a bounded temporary checkout on a Mac.
# This does not run a build or change the source repository.
remote_host="${1:-zachbook-pro}"
remote_dir="${2:-/tmp/beatglass-native-qualification}"
ref="${3:-HEAD}"

git rev-parse --verify "$ref" >/dev/null
printf 'Transferring %s to %s:%s\n' "$(git rev-parse "$ref")" "$remote_host" "$remote_dir"
git bundle create - "$ref" |
  ssh "$remote_host" "mkdir -p '$remote_dir' && cat > '$remote_dir/source.bundle'"
ssh "$remote_host" "git -C '$remote_dir' init -q && git -C '$remote_dir' fetch -q '$remote_dir/source.bundle' '$ref' && git -C '$remote_dir' checkout -q -f FETCH_HEAD && rm -f '$remote_dir/source.bundle'"
ssh "$remote_host" "git -C '$remote_dir' rev-parse HEAD"
printf 'Remote checkout prepared at %s:%s\n' "$remote_host" "$remote_dir"
