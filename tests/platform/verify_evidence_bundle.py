"""Verify a portable qualification evidence manifest and retained scenario files."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from datetime import datetime, timedelta, timezone
import sys
REQUIRED_SCENARIOS = {"capture", "recovery", "output", "display", "playback", "overlay"}

REQUIRED_PROVENANCE = ("platform", "commit", "captured_at", "environment")
SUPPORTED_PLATFORMS = {"linux", "windows", "macos", "fixture"}


def verify(root: Path, manifest: Path) -> list[str]:
    try:
        data = json.loads(manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"invalid manifest JSON: {error}"]
    if not isinstance(data, dict):
        return ["manifest root must be an object"]
    errors = []
    for field in REQUIRED_PROVENANCE:
        if not isinstance(data.get(field), str) or not data[field].strip():
            errors.append(f"missing provenance field: {field}")
    platform = data.get("platform")
    if isinstance(platform, str) and platform.strip():
        if platform != platform.lower():
            errors.append("platform must be lowercase")
        elif platform not in SUPPORTED_PLATFORMS:
            errors.append(f"unsupported platform: {platform}")
    environment = data.get("environment")
    if isinstance(environment, str) and len(environment) > 512:
        errors.append("environment exceeds 512 characters")
    if isinstance(environment, str) and environment.strip() and environment != environment.strip():
        errors.append("environment must not have leading or trailing whitespace")
    if isinstance(environment, str) and any(ord(char) < 32 and char not in "\t" for char in environment):
        errors.append("environment must not contain control characters")
    commit = data.get("commit")
    if isinstance(commit, str) and commit.strip() and not re.fullmatch(r"[0-9a-f]{40}", commit):
        errors.append("commit must be a 40-character hexadecimal SHA-1")
    captured_at = data.get("captured_at")
    if isinstance(captured_at, str) and captured_at.strip():
        try:
            parsed = datetime.fromisoformat(captured_at.replace("Z", "+00:00"))
            if parsed.tzinfo is None:
                errors.append("captured_at must include timezone")
            elif parsed > datetime.now(timezone.utc) + timedelta(minutes=5):
                errors.append("captured_at cannot be in the future")
        except ValueError:
            errors.append("captured_at must be ISO-8601")
    artifacts = data.get("artifacts")
    scenarios = data.get("scenarios")
    if not isinstance(artifacts, list) or not artifacts:
        errors.append("artifacts must be a nonempty list")
    elif len(artifacts) > 1000:
        errors.append("artifacts exceed 1000 entries")
    if not isinstance(scenarios, list) or not scenarios:
        errors.append("scenarios must be a nonempty list")
    scenario_ids = {
        scenario.get("id") for scenario in scenarios or []
        if isinstance(scenario, dict) and isinstance(scenario.get("id"), str)
    }
    missing_scenarios = REQUIRED_SCENARIOS - scenario_ids
    if missing_scenarios:
        errors.append(f"missing required scenarios: {sorted(missing_scenarios)}")
    unknown_scenarios = scenario_ids - REQUIRED_SCENARIOS
    if unknown_scenarios:
        errors.append(f"unknown scenarios: {sorted(unknown_scenarios)}")
    declared_ids = [scenario.get("id") for scenario in scenarios or [] if isinstance(scenario, dict)]
    duplicate_scenarios = sorted({
        scenario_id for scenario_id in declared_ids
        if isinstance(scenario_id, str) and declared_ids.count(scenario_id) > 1
    })
    if duplicate_scenarios:
        errors.append(f"duplicate scenarios: {duplicate_scenarios}")
    seen = set()
    for entry in artifacts or []:
        if not isinstance(entry, dict):
            errors.append(f"invalid artifact entry: {entry}")
            continue
        unknown_fields = set(entry) - {"path", "sha256", "role"}
        if unknown_fields:
            errors.append(f"unsupported artifact fields: {sorted(unknown_fields)}")
            continue
        relative = entry.get("path")
        raw_expected = entry.get("sha256", "")
        expected = raw_expected.lower() if isinstance(raw_expected, str) else None
        canonical = Path(relative).as_posix() if isinstance(relative, str) else None
        if (
            not isinstance(relative, str)
            or not isinstance(expected, str)
            or raw_expected != expected
            or len(relative) > 512
            or relative != relative.strip()
            or "\\" in relative
            or any(part == "." for part in relative.split("/"))
            or "//" in relative
            or relative.endswith("/")
            or not re.fullmatch(r"[0-9a-f]{64}", expected)
            or (isinstance(relative, str) and "\x00" in relative)
            or not canonical
            or canonical != relative
            or canonical in seen
        ):
            errors.append(f"invalid or duplicate artifact entry: {entry}"); continue
        if "role" in entry and (not isinstance(entry["role"], str) or not entry["role"].strip()):
            errors.append(f"artifact role must be a nonempty string: {relative}"); continue
        if "role" in entry and entry["role"] != entry["role"].strip():
            errors.append(f"artifact role must not have leading or trailing whitespace: {relative}"); continue
        if "role" in entry and len(entry["role"]) > 128:
            errors.append(f"artifact role exceeds 128 characters: {relative}"); continue
        seen.add(canonical)
        path = (root / canonical).resolve()
        if root.resolve() not in path.parents:
            errors.append(f"artifact escapes bundle: {relative}"); continue
        if not path.is_file():
            errors.append(f"missing artifact: {relative}"); continue
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != expected:
            errors.append(f"hash mismatch: {relative}")
    evidence_owners = {}
    artifact_roles = {
        entry.get("path"): entry.get("role") for entry in artifacts or []
        if isinstance(entry, dict) and isinstance(entry.get("path"), str)
    }
    for scenario in scenarios or []:
        if (not isinstance(scenario, dict) or not isinstance(scenario.get("id"), str)
                or not scenario.get("id") or not isinstance(scenario.get("status"), str)
                or scenario.get("status") not in {"PASS", "FAIL", "UNRUN", "PENDING"}):
            errors.append(f"invalid scenario: {scenario}"); continue
        if scenario["status"] not in {"UNRUN", "PENDING"}:
            evidence = scenario.get("evidence")
            if not isinstance(evidence, str) or not evidence or evidence not in seen:
                errors.append(f"scenario evidence is not retained: {scenario.get('id')}")
            elif evidence in evidence_owners:
                errors.append(f"scenario evidence is shared: {evidence} ({evidence_owners[evidence]}, {scenario.get('id')})")
            elif not artifact_roles.get(evidence):
                errors.append(f"scenario evidence role is missing: {scenario.get('id')}")
            elif artifact_roles.get(evidence) != scenario.get("id"):
                errors.append(f"scenario evidence role mismatch: {scenario.get('id')} -> {evidence}")
            else:
                evidence_owners[evidence] = scenario.get("id")
        elif scenario.get("evidence"):
            errors.append(f"incomplete scenario must not claim evidence: {scenario.get('id')}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    errors = verify(args.root, args.manifest)
    if errors:
        print("\n".join(errors), file=sys.stderr); return 1
    print("evidence bundle verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
