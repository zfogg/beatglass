#!/usr/bin/env bash
set -euo pipefail

out_dir="${1:?usage: verify_qualification_artifacts.sh <qualification-output-dir>}"
manifest="$out_dir/qualification-manifest.txt"
hashes="$out_dir/qualification-artifacts.sha256"
case_matrix="$out_dir/qualification-cases.tsv"
[[ -s "$manifest" ]] || { echo "missing qualification manifest: $manifest" >&2; exit 1; }
[[ -s "$hashes" ]] || { echo "missing artifact hash manifest: $hashes" >&2; exit 1; }
[[ -s "$case_matrix" ]] || { echo "missing qualification case matrix: $case_matrix" >&2; exit 1; }
for key in commit macos_version architecture xcode_version sdk_version sdk_path developer_dir artifact_sha256; do
  grep -q "^${key}=." "$manifest" || { echo "manifest missing ${key}" >&2; exit 1; }
done
expected_cases=(permission_prompt capture_nonzero_timestamp permission_revoke_recovery output_switch_uid fifo_exhaustion_recovery overlay_visibility overlay_click_through_recovery overlay_display_change overlay_fullscreen_spaces menubar_dock_reopen)
mapfile -t case_lines < "$case_matrix"
[[ "${case_lines[0]:-}" == $'case_id\tstatus\tevidence' ]] || { echo "invalid qualification case matrix header" >&2; exit 1; }
[[ "${#case_lines[@]}" -eq 11 ]] || { echo "expected exactly ten qualification cases" >&2; exit 1; }
for i in "${!expected_cases[@]}"; do
  IFS=$'\t' read -r case_id status evidence <<< "${case_lines[$((i + 1))]}"
  [[ "$case_id" == "${expected_cases[$i]}" ]] || { echo "unexpected qualification case: $case_id" >&2; exit 1; }
  [[ "$status" =~ ^(PASS|FAIL|UNRUN)$ ]] || { echo "invalid status for $case_id: $status" >&2; exit 1; }
  if [[ "$status" == PASS && -z "$evidence" ]]; then echo "PASS case lacks evidence: $case_id" >&2; exit 1; fi
done
command -v shasum >/dev/null 2>&1 || { echo "shasum is required" >&2; exit 1; }
mapfile -t hash_lines < "$hashes"
[[ "${#hash_lines[@]}" -eq 3 ]] || { echo "expected exactly three artifact hashes" >&2; exit 1; }
declare -A seen=()
for line in "${hash_lines[@]}"; do
  [[ "$line" =~ ^([[:xdigit:]]{64})[[:space:]][[:space:]](.+)$ ]] || {
    echo "malformed artifact hash line: $line" >&2; exit 1;
  }
  expected="${BASH_REMATCH[1]}"
  relative="${BASH_REMATCH[2]}"
  [[ "$relative" != /* && "$relative" != *$'\n'* ]] || {
    echo "artifact path must be relative: $relative" >&2; exit 1;
  }
  candidate=$(cd "$out_dir" && cd "$(dirname -- "$relative")" 2>/dev/null && pwd)/$(basename -- "$relative") || {
    echo "invalid artifact path: $relative" >&2; exit 1;
  }
  root=$(cd "$out_dir" && pwd)
  case "$candidate" in
    "$root"/*) ;;
    *) echo "artifact path escapes output: $relative" >&2; exit 1;;
  esac
  [[ -z "${seen[$candidate]:-}" ]] || { echo "duplicate artifact path: $relative" >&2; exit 1; }
  seen[$candidate]=1
  [[ -f "$candidate" ]] || { echo "missing artifact: $candidate" >&2; exit 1; }
  actual=$(shasum -a 256 "$candidate" | awk '{print $1}')
  [[ "$actual" == "$expected" ]] || {
    echo "hash mismatch for $relative (expected $expected, got $actual)" >&2; exit 1;
  }
done
echo "qualification artifact hashes verified: $out_dir"
