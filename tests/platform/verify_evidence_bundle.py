"""Verify a portable qualification evidence manifest and retained scenario files."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
REQUIRED_SCENARIOS = {"capture", "recovery", "output", "display", "playback", "overlay"}

REQUIRED_PROVENANCE = ("platform", "commit", "captured_at", "environment")


def verify(root: Path, manifest: Path) -> list[str]:
    data = json.loads(manifest.read_text(encoding="utf-8"))
    errors = []
    for field in REQUIRED_PROVENANCE:
        if not isinstance(data.get(field), str) or not data[field].strip():
            errors.append(f"missing provenance field: {field}")
    artifacts = data.get("artifacts")
    scenarios = data.get("scenarios")
    if not isinstance(artifacts, list) or not artifacts:
        errors.append("artifacts must be a nonempty list")
    if not isinstance(scenarios, list) or not scenarios:
        errors.append("scenarios must be a nonempty list")
    scenario_ids = {scenario.get("id") for scenario in scenarios or [] if isinstance(scenario, dict)}
    missing_scenarios = REQUIRED_SCENARIOS - scenario_ids
    if missing_scenarios:
        errors.append(f"missing required scenarios: {sorted(missing_scenarios)}")
    seen = set()
    for entry in artifacts or []:
        relative, expected = entry.get("path"), entry.get("sha256", "").lower()
        if not isinstance(relative, str) or not isinstance(expected, str) or relative in seen:
            errors.append(f"invalid or duplicate artifact entry: {entry}"); continue
        seen.add(relative)
        path = (root / relative).resolve()
        if root.resolve() not in path.parents:
            errors.append(f"artifact escapes bundle: {relative}"); continue
        if not path.is_file():
            errors.append(f"missing artifact: {relative}"); continue
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != expected:
            errors.append(f"hash mismatch: {relative}")
    evidence_owners = {}
    for scenario in scenarios or []:
        if not isinstance(scenario, dict) or not scenario.get("id") or scenario.get("status") not in {"PASS", "FAIL", "UNRUN"}:
            errors.append(f"invalid scenario: {scenario}"); continue
        if scenario["status"] != "UNRUN":
            evidence = scenario.get("evidence")
            if not evidence or evidence not in seen:
                errors.append(f"scenario evidence is not retained: {scenario.get('id')}")
            elif evidence in evidence_owners:
                errors.append(f"scenario evidence is shared: {evidence} ({evidence_owners[evidence]}, {scenario.get('id')})")
            else:
                evidence_owners[evidence] = scenario.get("id")
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
