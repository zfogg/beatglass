"""Verify an exported native PipeWire evidence archive independently."""
import hashlib
import json
import sys
import zipfile
from datetime import datetime, timezone
import xml.etree.ElementTree as ET
from pathlib import PurePosixPath

def count(node, key):
    """Read a non-negative JUnit count without accepting malformed metadata."""
    try:
        value = int(node.attrib.get(key, "0"))
    except (TypeError, ValueError):
        raise SystemExit(f"JUnit evidence has malformed {key} count")
    if value < 0:
        raise SystemExit(f"JUnit evidence has negative {key} count")
    return value

if len(sys.argv) != 2:
    raise SystemExit("usage: verify_native_evidence_archive.py EVIDENCE.zip")
archive_path = sys.argv[1]
with zipfile.ZipFile(archive_path) as archive:
    raw_names = archive.namelist()
    if len(raw_names) != len(set(raw_names)):
        raise SystemExit("duplicate archive entry")
    if any(name.endswith("/") or name.startswith("/") or "/" in name or "\\" in name or any(ord(char) < 32 for char in name) or ".." in PurePosixPath(name).parts for name in raw_names):
        raise SystemExit("unsafe archive entry name")
    names = set(raw_names)
    if len({name.casefold() for name in raw_names}) != len(raw_names):
        raise SystemExit("case-insensitive archive entry collision")
    if "evidence-manifest.json" not in names:
        raise SystemExit("missing evidence-manifest.json")
    try:
        manifest = json.loads(archive.read("evidence-manifest.json"))
    except (json.JSONDecodeError, UnicodeDecodeError):
        raise SystemExit("malformed evidence manifest")
    if not isinstance(manifest, dict):
        raise SystemExit("evidence manifest is not an object")
    if manifest.get("format") != "beatglass-native-pipewire-evidence-v1" or manifest.get("qualified") is not True:
        raise SystemExit("invalid or unqualified evidence manifest")
    if not isinstance(manifest.get("commit"), str) or not manifest["commit"].strip():
        raise SystemExit("evidence manifest has no commit identity")
    if (not isinstance(manifest.get("source_date_epoch"), int) or isinstance(manifest.get("source_date_epoch"), bool) or
            manifest["source_date_epoch"] < 0 or not isinstance(manifest.get("archive_timestamp_utc"), str) or
            not manifest["archive_timestamp_utc"].endswith("Z")):
        raise SystemExit("evidence manifest has invalid reproducibility metadata")
    try:
        datetime.fromisoformat(manifest["archive_timestamp_utc"][:-1])
    except ValueError:
        raise SystemExit("evidence manifest has malformed archive timestamp")
    try:
        expected_archive_time = datetime.fromtimestamp(manifest["source_date_epoch"], timezone.utc).replace(tzinfo=None)
    except (OverflowError, OSError, ValueError):
        raise SystemExit("evidence manifest has unrepresentable source date epoch")
    expected_archive_time = max(expected_archive_time, datetime(1980, 1, 1))
    if manifest["archive_timestamp_utc"] != expected_archive_time.isoformat() + "Z":
        raise SystemExit("archive timestamp does not match source date epoch")
    artifacts = manifest.get("artifacts", [])
    if not isinstance(artifacts, list) or not artifacts:
        raise SystemExit("evidence manifest contains no artifacts")
    if any(not isinstance(artifact, dict) or not isinstance(artifact.get("name"), str) or not artifact["name"].strip() or artifact["name"] in (".", "..", "evidence-manifest.json") or "/" in artifact["name"] or "\\" in artifact["name"] or any(ord(char) < 32 for char in artifact["name"]) for artifact in artifacts):
        raise SystemExit("malformed manifest artifact name")
    if len({artifact["name"] for artifact in artifacts}) != len(artifacts):
        raise SystemExit("duplicate manifest artifact")
    if len({artifact["name"].casefold() for artifact in artifacts}) != len(artifacts):
        raise SystemExit("case-insensitive duplicate manifest artifact")
    declared = {artifact["name"] for artifact in artifacts}
    unexpected = names - declared - {"evidence-manifest.json"}
    if unexpected:
        raise SystemExit(f"unlisted archive artifact: {sorted(unexpected)[0]}")
    summary_names = [name for name in declared if name.endswith(".json") and "summary" in name]
    try:
        summary_data = json.loads(archive.read(summary_names[0])) if len(summary_names) == 1 else {}
    except (json.JSONDecodeError, UnicodeDecodeError):
        raise SystemExit("malformed qualification summary")
    if not isinstance(summary_data, dict):
        raise SystemExit("qualification summary is not an object")
    if "commit" in summary_data and (not isinstance(summary_data["commit"], str) or not summary_data["commit"].strip()):
        raise SystemExit("qualification summary has invalid commit identity")
    if len(summary_names) != 1 or summary_data.get("commit") != manifest.get("commit"):
        raise SystemExit("manifest commit does not match qualification summary")
    if "platform" in summary_data and (not isinstance(summary_data["platform"], str) or not summary_data["platform"].strip()):
        raise SystemExit("qualification summary has invalid platform identity")
    if "platform" in manifest and (not isinstance(manifest["platform"], str) or not manifest["platform"].strip()):
        raise SystemExit("evidence manifest has invalid platform identity")
    if manifest.get("platform") and manifest.get("platform") != summary_data.get("platform"):
        raise SystemExit("manifest platform does not match qualification summary")
    if manifest.get("capture_plan"):
        if (not isinstance(manifest["capture_plan"], str) or not manifest["capture_plan"].strip() or
                manifest.get("capture_plan_validated") is not True):
            raise SystemExit("invalid capture plan provenance")
        capture_name = manifest["capture_plan"].replace("\\", "/").rsplit("/", 1)[-1]
        if capture_name not in declared:
            raise SystemExit("capture plan is not archived")
        summary_capture = summary_data.get("capture_plan")
        if (not isinstance(summary_capture, str) or
                summary_capture.replace("\\", "/").rsplit("/", 1)[-1] != capture_name):
            raise SystemExit("summary capture plan path does not match archive")
        if (not isinstance(manifest.get("capture_plan_sha256"), str) or len(manifest["capture_plan_sha256"]) != 64 or
                manifest["capture_plan_sha256"] != summary_data.get("capture_plan_sha256")):
            raise SystemExit("capture plan digest provenance mismatch")
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
        if (not isinstance(artifact.get("size"), int) or isinstance(artifact.get("size"), bool) or artifact["size"] < 0 or
                not isinstance(artifact.get("sha256"), str) or len(artifact["sha256"]) != 64 or any(char not in "0123456789abcdef" for char in artifact["sha256"])):
            raise SystemExit(f"malformed artifact metadata: {name}")
        if name not in names:
            raise SystemExit(f"manifest artifact missing from archive: {name}")
        payload = archive.read(name)
        if len(payload) != artifact["size"]:
            raise SystemExit(f"size mismatch: {name}")
        if hashlib.sha256(payload).hexdigest() != artifact["sha256"]:
            raise SystemExit(f"digest mismatch: {name}")
    if manifest.get("capture_plan"):
        capture_name = manifest["capture_plan"].replace("\\", "/").rsplit("/", 1)[-1]
        if summary_data["capture_plan_sha256"] != hashlib.sha256(archive.read(capture_name)).hexdigest():
            raise SystemExit("summary capture plan digest does not match archive")
    discovery_name = next(name for name in declared if name.endswith(".json") and "discovery" in name)
    junit_name = next(name for name in declared if (name.endswith(".xml") and "junit" in name) or name.endswith("tests.xml"))
    if not isinstance(summary_data.get("native_discovery_json"), str) or summary_data["native_discovery_json"].replace("\\", "/").rsplit("/", 1)[-1] != discovery_name:
        raise SystemExit("summary discovery path does not match archive")
    if not isinstance(summary_data.get("native_junit"), str) or summary_data["native_junit"].replace("\\", "/").rsplit("/", 1)[-1] != junit_name:
        raise SystemExit("summary JUnit path does not match archive")
    if summary_data.get("native_discovery_sha256") != hashlib.sha256(archive.read(discovery_name)).hexdigest():
        raise SystemExit("summary native discovery digest does not match archive")
    if summary_data.get("native_junit_sha256") != hashlib.sha256(archive.read(junit_name)).hexdigest():
        raise SystemExit("summary native JUnit digest does not match archive")
    transition_path = summary_data.get("native_transition_json")
    if transition_path:
        if not isinstance(transition_path, str) or not transition_path.strip():
            raise SystemExit("summary transition path is invalid")
        transition_name = transition_path.replace("\\", "/").rsplit("/", 1)[-1]
        if transition_name not in declared:
            raise SystemExit("summary transition path does not match archive")
        transition_payload = archive.read(transition_name)
        if summary_data.get("native_transition_sha256") != hashlib.sha256(transition_payload).hexdigest():
            raise SystemExit("summary transition digest does not match archive")
        try:
            transitions = json.loads(transition_payload)
        except (json.JSONDecodeError, UnicodeDecodeError):
            raise SystemExit("malformed transition evidence")
        if not isinstance(transitions, dict) or transitions.get("schema") != "beatglass-pipewire-transitions-v1":
            raise SystemExit("transition evidence has invalid provenance schema")
        if manifest.get("transition_schema") != transitions["schema"]:
            raise SystemExit("transition schema does not match manifest")
        if not isinstance(transitions.get("commit"), str) or not transitions["commit"].strip():
            raise SystemExit("transition evidence has no provenance commit")
        if transitions["commit"] != manifest.get("commit"):
            raise SystemExit("transition evidence commit does not match archive manifest")
        if manifest.get("transition_commit") != transitions["commit"]:
            raise SystemExit("transition commit does not match manifest")
        if not isinstance(transitions.get("platform"), str) or not transitions["platform"].strip():
            raise SystemExit("transition evidence has no provenance platform")
        if summary_data.get("platform") and transitions["platform"] != summary_data["platform"]:
            raise SystemExit("transition evidence platform does not match summary")
        if not transitions["platform"].lower().startswith("linux"):
            raise SystemExit("transition evidence is not from a Linux host")
        transitions = transitions.get("events")
        if not isinstance(transitions, list) or not transitions:
            raise SystemExit("transition evidence is empty")
        if (not isinstance(summary_data.get("native_transition_event_count"), int) or
                isinstance(summary_data.get("native_transition_event_count"), bool) or
                summary_data["native_transition_event_count"] <= 0 or
                summary_data["native_transition_event_count"] != len(transitions)):
            raise SystemExit("transition event count does not match summary")
        manifest_count = manifest.get("transition_event_count")
        if not isinstance(manifest_count, int) or isinstance(manifest_count, bool) or manifest_count <= 0 or manifest_count != len(transitions):
            raise SystemExit("transition event count does not match manifest")
        names = [event.get("event") for event in transitions if isinstance(event, dict)]
        if any(required not in names for required in ("start", "process", "error", "restart")):
            raise SystemExit("transition evidence is missing lifecycle events")
        epochs = [event.get("epoch") for event in transitions if isinstance(event, dict) and event.get("epoch") is not None]
        if any(not isinstance(epoch, int) or epoch <= 0 for epoch in epochs) or epochs != sorted(epochs):
            raise SystemExit("transition evidence epochs are not monotonic")
        if not any(event.get("discontinuity") is True for event in transitions if isinstance(event, dict)):
            raise SystemExit("transition evidence has no discontinuity")
        if not any(event.get("error_code", 0) != 0 for event in transitions if isinstance(event, dict)):
            raise SystemExit("transition evidence has no error code")
        start = next(event for event in transitions if event.get("event") == "start")
        if not isinstance(start.get("serial"), str) or not start["serial"].strip():
            raise SystemExit("transition evidence has no sink serial")
        if not any(isinstance(event.get("sample_count"), int) and event["sample_count"] > 0
                   for event in transitions if event.get("event") == "process"):
            raise SystemExit("transition evidence has no captured samples")
        restart = next(event for event in transitions if event.get("event") == "restart")
        fmt = restart.get("format")
        if not isinstance(fmt, dict) or fmt.get("sample_rate", 0) <= 0 or fmt.get("channels", 0) <= 0:
            raise SystemExit("transition evidence has no negotiated format")
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
    if int(summary_data["native_test_discovery"].get("count", len(discovery_tests))) != len(discovery_tests):
        raise SystemExit("summary native discovery count does not match discovery")
    try:
        junit_root = ET.fromstring(archive.read(junit_name))
    except ET.ParseError as error:
        raise SystemExit(f"JUnit evidence is malformed XML: {error}")
    if junit_root.tag not in ("testsuite", "testsuites"):
        raise SystemExit("JUnit evidence has invalid root")
    suites = [junit_root] if junit_root.tag == "testsuite" else [node for node in junit_root.iter("testsuite")]
    if not suites or sum(count(node, "tests") for node in suites) < 1:
        raise SystemExit("JUnit evidence has no executed tests")
    if junit_root.tag == "testsuites" and count(junit_root, "tests") != sum(count(node, "tests") for node in suites):
        raise SystemExit("JUnit aggregate test count does not match suites")
    if junit_root.tag == "testsuites":
        for key, label in (("failures", "failure"), ("errors", "error"), ("skipped", "skipped")):
            if count(junit_root, key) != sum(count(node, key) for node in suites):
                raise SystemExit(f"JUnit aggregate {label} count does not match suites")
    if sum(count(node, "failures") for node in suites) or sum(count(node, "errors") for node in suites) or any(node.tag in ("failure", "error") for node in junit_root.iter()):
        raise SystemExit("JUnit evidence reports failures or errors")
    if sum(count(node, "skipped") for node in suites) or any(node.tag == "skipped" for node in junit_root.iter()):
        raise SystemExit("JUnit evidence reports skipped tests")
    testcases = [node for node in junit_root.iter() if node.tag == "testcase"]
    if not testcases or any(not node.attrib.get("name", "").strip() for node in testcases):
        raise SystemExit("JUnit evidence has unnamed testcases")
    testcase_names = [node.attrib["name"] for node in testcases]
    if len(testcase_names) != len(set(testcase_names)):
        raise SystemExit("JUnit evidence has duplicate testcase names")
    if int(summary_data.get("native_junit_test_count", 0)) != len(testcases):
        raise SystemExit("summary native_junit_test_count does not match JUnit")
    if sum(count(node, "tests") for node in suites) != len(testcases):
        raise SystemExit("JUnit suite test counts do not match testcases")
    for suite in suites:
        if count(suite, "tests") != sum(1 for node in suite.iter("testcase")):
            raise SystemExit("JUnit suite test count does not match its testcases")
        if count(suite, "failures") + count(suite, "errors") + count(suite, "skipped") > count(suite, "tests"):
            raise SystemExit("JUnit suite status counts exceed tests")
    if set(discovery_names) != set(testcase_names):
        raise SystemExit("discovery and JUnit test names do not match")
print("Native PipeWire evidence archive verified")
