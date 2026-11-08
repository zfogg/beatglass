#!/usr/bin/env bash
set -euo pipefail

out_dir="${1:?usage: verify_qualification_artifacts.sh <qualification-output-dir>}"
manifest="$out_dir/qualification-manifest.txt"
hashes="$out_dir/qualification-artifacts.sha256"
[[ -s "$manifest" ]] || { echo "missing qualification manifest: $manifest" >&2; exit 1; }
[[ -s "$hashes" ]] || { echo "missing artifact hash manifest: $hashes" >&2; exit 1; }
for key in commit macos_version architecture xcode_version sdk_version sdk_path developer_dir artifact_sha256; do
  grep -q "^${key}=." "$manifest" || { echo "manifest missing ${key}" >&2; exit 1; }
done
[[ "$(wc -l < "$hashes" | tr -d ' ')" -eq 3 ]] || { echo "expected exactly three artifact hashes" >&2; exit 1; }
command -v shasum >/dev/null 2>&1 || { echo "shasum is required" >&2; exit 1; }
(cd "$out_dir" && shasum -a 256 -c "$(basename "$hashes")")
echo "qualification artifact hashes verified: $out_dir"
