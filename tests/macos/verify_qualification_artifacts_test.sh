#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
verifier="$repo_root/tests/macos/verify_qualification_artifacts.sh"
if ! command -v shasum >/dev/null 2>&1; then
  echo "SKIP: shasum is required for macOS verifier regression"
  exit 0
fi
out_dir=$(mktemp -d "${TMPDIR:-/tmp}/beatglass-verify.XXXXXX")
trap 'rm -rf "$out_dir"' EXIT
mkdir -p "$out_dir/build/beatglass-desktop.app/Contents/MacOS" "$out_dir/build/beatglass-coreaudio-smoke.app/Contents/MacOS" "$out_dir/build/beatglass-coreaudio-recovery.app/Contents/MacOS"
printf desktop > "$out_dir/build/beatglass-desktop.app/Contents/MacOS/beatglass-desktop"
printf smoke > "$out_dir/build/beatglass-coreaudio-smoke.app/Contents/MacOS/beatglass-coreaudio-smoke"
printf recovery > "$out_dir/build/beatglass-coreaudio-recovery.app/Contents/MacOS/beatglass-coreaudio-recovery"
cat > "$out_dir/qualification-manifest.txt" <<'EOF'
commit=test
macos_version=test
architecture=x86_64
xcode_version=test
sdk_version=test
sdk_path=/sdk
developer_dir=/dev
artifact_sha256=qualification-artifacts.sha256
case_matrix=qualification-cases.tsv
EOF
cat > "$out_dir/qualification-cases.tsv" <<'EOF'
case_id	status	evidence
permission_prompt	UNRUN	-
capture_nonzero_timestamp	UNRUN	-
permission_revoke_recovery	UNRUN	-
output_switch_uid	UNRUN	-
fifo_exhaustion_recovery	UNRUN	-
overlay_visibility	UNRUN	-
overlay_click_through_recovery	UNRUN	-
overlay_display_change	UNRUN	-
overlay_fullscreen_spaces	UNRUN	-
menubar_dock_reopen	UNRUN	-
EOF
(cd "$out_dir" && shasum -a 256 build/beatglass-desktop.app/Contents/MacOS/beatglass-desktop build/beatglass-coreaudio-smoke.app/Contents/MacOS/beatglass-coreaudio-smoke build/beatglass-coreaudio-recovery.app/Contents/MacOS/beatglass-coreaudio-recovery) > "$out_dir/qualification-artifacts.sha256"
"$verifier" "$out_dir"
sed -i '1s/../00/' "$out_dir/qualification-artifacts.sha256"
if "$verifier" "$out_dir"; then
  echo "tampered hash unexpectedly passed" >&2
  exit 1
fi
echo "macOS qualification verifier regression checks passed"
