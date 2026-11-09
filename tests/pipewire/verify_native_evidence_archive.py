"""Verify an exported native PipeWire evidence archive independently."""
import hashlib
import json
import sys
import zipfile
from datetime import datetime
import xml.etree.ElementTree as ET
from pathlib import PurePosixPath

if len(sys.argv) != 2:
    raise SystemExit("usage: verify_native_evidence_archive.py EVIDENCE.zip")
archive_path = sys.argv[1]
with zipfile.ZipFile(archive_path) as archive:
    raw_names = archive.namelist()
    if len(raw_names) != len(set(raw_names)):
        raise SystemExit("duplicate archive entry")
    if any(name.endswith("/") or name.startswith("/") or ".." in PurePosixPath(name).parts for name in raw_names):
        raise SystemExit("unsafe archive entry name")
    names = set(raw_names)
    if "evidence-manifest.json" not in names:
        raise SystemExit("missing evidence-manifest.json")
    manifest = json.loads(archive.read("evidence-manifest.json"))
    if manifest.get("format") != "beatglass-native-pipewire-evidence-v1" or not manifest.get("qualified"):
        raise SystemExit("invalid or unqualified evidence manifest")
    if not isinstance(manifest.get("commit"), str) or not manifest["commit"].strip():
        raise SystemExit("evidence manifest has no commit identity")
    if not isinstance(manifest.get("source_date_epoch"), int) or not manifest.get("archive_timestamp_utc", "").endswith("Z"):
        raise SystemExit("evidence manifest has invalid reproducibility metadata")
    try:
        datetime.fromisoformat(manifest["archive_timestamp_utc"][:-1])
    except ValueError:
        raise SystemExit("evidence manifest has malformed archive timestamp")
    artifacts = manifest.get("artifacts", [])
    if not artifacts:
        raise SystemExit("evidence manifest contains no artifacts")
    if len({artifact.get("name") for artifact in artifacts}) != len(artifacts):
        raise SystemExit("duplicate manifest artifact")
    declared = {artifact["name"] for artifact in artifacts}
    unexpected = names - declared - {"evidence-manifest.json"}
    if unexpected:
        raise SystemExit(f"unlisted archive artifact: {sorted(unexpected)[0]}")
    summary_names = [name for name in declared if name.endswith(".json") and "summary" in name]
    summary_data = json.loads(archive.read(summary_names[0])) if len(summary_names) == 1 else {}
    if len(summary_names) != 1 or summary_data.get("commit") != manifest.get("commit"):
        raise SystemExit("manifest commit does not match qualification summary")
    if summary_data.get("qualified") is not True:
        raise SystemExit("qualification summary is not qualified")
    if summary_data.get("native_test_name_match") is not True:
        raise SystemExit("summary does not confirm native test-name match")
    if any(int(summary_data.get(key, -1)) != 0 for key in ("native_junit_failures", "native_junit_errors", "native_junit_skipped")):
        raise SystemExit("summary reports JUnit failures, errors, or skips")
    if summary_data.get("native_junit_exists") is not True:
        raise SystemExit("summary does not confirm native JUnit existence")
    if not isinstance(summary_data.get("native_test_discovery"), dict) or not summary_data["native_test_discovery"].get("tests"):
        raise SystemExit("summary has no native test discovery record")
    if not any((name.endswith(".xml") and "junit" in name) or name.endswith("tests.xml") for name in declared):
        raise SystemExit("evidence archive has no JUnit results")
    if not any(name.endswith(".json") and "discovery" in name for name in declared):
        raise SystemExit("evidence archive has no CTest discovery JSON")
    for artifact in artifacts:
        name = artifact["name"]
        if name not in names:
            raise SystemExit(f"manifest artifact missing from archive: {name}")
        payload = archive.read(name)
        if len(payload) != artifact["size"]:
            raise SystemExit(f"size mismatch: {name}")
        if hashlib.sha256(payload).hexdigest() != artifact["sha256"]:
            raise SystemExit(f"digest mismatch: {name}")
    discovery_name = next(name for name in declared if name.endswith(".json") and "discovery" in name)
    discovery_tests = json.loads(archive.read(discovery_name)).get("tests")
    if not isinstance(discovery_tests, list):
        raise SystemExit("discovery JSON has no tests list")
    if not discovery_tests:
        raise SystemExit("discovery JSON has no registered tests")
    if any(not isinstance(test, dict) or not str(test.get("name", "")).strip() for test in discovery_tests):
        raise SystemExit("discovery JSON has an unnamed test")
    discovery_names = [test["name"] for test in discovery_tests]
    if len(discovery_names) != len(set(discovery_names)):
        raise SystemExit("discovery JSON has duplicate test names")
    if int(summary_data.get("native_test_count", 0)) != len(discovery_tests):
        raise SystemExit("summary native_test_count does not match discovery")
    junit_name = next(name for name in declared if (name.endswith(".xml") and "junit" in name) or name.endswith("tests.xml"))
    junit_root = ET.fromstring(archive.read(junit_name))
    if junit_root.tag not in ("testsuite", "testsuites"):
        raise SystemExit("JUnit evidence has invalid root")
    if junit_root.tag == "testsuite" and int(junit_root.attrib.get("tests", "0")) < 1:
        raise SystemExit("JUnit evidence has no executed tests")
    if int(junit_root.attrib.get("failures", "0")) or int(junit_root.attrib.get("errors", "0")) or any(node.tag in ("failure", "error") for node in junit_root.iter()):
        raise SystemExit("JUnit evidence reports failures or errors")
    if int(junit_root.attrib.get("skipped", "0")) or any(node.tag == "skipped" for node in junit_root.iter()):
        raise SystemExit("JUnit evidence reports skipped tests")
    testcases = [node for node in junit_root.iter() if node.tag == "testcase"]
    if not testcases or any(not node.attrib.get("name", "").strip() for node in testcases):
        raise SystemExit("JUnit evidence has unnamed testcases")
    testcase_names = [node.attrib["name"] for node in testcases]
    if len(testcase_names) != len(set(testcase_names)):
        raise SystemExit("JUnit evidence has duplicate testcase names")
    if int(summary_data.get("native_junit_test_count", 0)) != len(testcases):
        raise SystemExit("summary native_junit_test_count does not match JUnit")
    if set(discovery_names) != set(testcase_names):
        raise SystemExit("discovery and JUnit test names do not match")
print("Native PipeWire evidence archive verified")
