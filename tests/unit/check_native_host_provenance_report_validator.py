from pathlib import Path
import json
import subprocess
import tempfile

source = Path("tests/pipewire/validate_native_host_provenance_report.py").read_text()
assert "host provenance report has an unsupported schema" in source
assert "host provenance report has invalid checked fields" in source
assert "host provenance report omits required provenance fields" in source
print("Native host provenance report validator verified")

for arguments in ([], ["one.json", "two.json"]):
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", *arguments], capture_output=True, text=True)
    assert result.returncode != 0
    assert "usage: validate_native_host_provenance_report.py REPORT.json" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "invalid-report.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": False, "platform": "linux", "name": "host", "checked_fields": ["name", "platform"]}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report is not valid" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "blank-name-report.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "  ", "checked_fields": ["name", "platform"]}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report has no name" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    for name in (None, 7, ["host"]):
        report = Path(directory) / "invalid-name-report.json"
        report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": name, "checked_fields": ["name", "platform"]}), encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance report has no name" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    for platform in (None, 7, "linux "):
        report = Path(directory) / "invalid-platform-report.json"
        report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": platform, "name": "host", "checked_fields": ["name", "platform"]}), encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance report has an invalid platform" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    for fields in (None, "name", {"name": True}):
        report = Path(directory) / "invalid-fields-report.json"
        report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "host", "checked_fields": fields}), encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance report has invalid checked fields" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "empty-fields-report.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "host", "checked_fields": []}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "omits required provenance fields" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    for fields in (["platform"], ["name"], ["node_id"]):
        report = Path(directory) / "missing-required-report.json"
        report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "host", "checked_fields": fields}), encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "omits required provenance fields" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "unsorted-fields-report.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "host", "checked_fields": ["platform", "name"]}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "invalid checked fields" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "padded-entry-report.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "host", "checked_fields": ["name", "platform", " node_id"]}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "invalid checked fields" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    for schema in (None, 1, "beatglass-native-host-provenance-v0"):
        report = Path(directory) / "invalid-schema-report.json"
        report.write_text(json.dumps({"schema": schema, "valid": True, "platform": "linux", "name": "host", "checked_fields": ["name", "platform"]}), encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "unsupported schema" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "whitespace-entry-report.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "host", "checked_fields": ["name", "platform", " "]}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "invalid checked fields" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "empty-entry-report.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": "linux", "name": "host", "checked_fields": ["", "name", "platform"]}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "invalid checked fields" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "truthy-valid.json"
    report.write_text(json.dumps({"schema": "beatglass-native-host-provenance-v1", "valid": 1, "platform": "linux", "name": "host", "checked_fields": ["name", "platform"]}), encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report is not valid" in result.stderr

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "invalid-report.json"
    report.write_text(json.dumps({
        "schema": "beatglass-native-host-provenance-v1",
        "valid": True,
        "platform": "linux",
        "name": "qualification-host",
        "checked_fields": ["name"],
    }), encoding="utf-8")
    result = subprocess.run(
        ["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "required provenance fields" in result.stderr
print("Native host provenance report rejection verified")

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "duplicate-fields.json"
    report.write_text(json.dumps({
        "schema": "beatglass-native-host-provenance-v1",
        "valid": True,
        "platform": "linux",
        "name": "qualification-host",
        "checked_fields": ["name", "name", "platform"],
    }), encoding="utf-8")
    result = subprocess.run(
        ["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "invalid checked fields" in result.stderr
print("Native host provenance duplicate-field rejection verified")

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "non-string-fields.json"
    report.write_text(json.dumps({
        "schema": "beatglass-native-host-provenance-v1",
        "valid": True,
        "platform": "linux",
        "name": "qualification-host",
        "checked_fields": ["name", ["platform"]],
    }), encoding="utf-8")
    result = subprocess.run(
        ["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "invalid checked fields" in result.stderr
print("Native host provenance type rejection verified")

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "unknown-field.json"
    report.write_text(json.dumps({
        "schema": "beatglass-native-host-provenance-v1",
        "valid": True,
        "platform": "linux",
        "name": "qualification-host",
        "checked_fields": ["name", "platform"],
        "unexpected": "not part of the report contract",
    }), encoding="utf-8")
    result = subprocess.run(
        ["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "unsupported fields" in result.stderr
print("Native host provenance unknown-field rejection verified")

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "malformed-report.json"
    report.write_text("{not-json", encoding="utf-8")
    result = subprocess.run(
        ["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "report is unreadable" in result.stderr
print("Native host provenance malformed-input rejection verified")

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "invalid-utf8-report.json"
    report.write_bytes(b"{\xff\xfe")
    result = subprocess.run(
        ["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "report is unreadable" in result.stderr
print("Native host provenance encoding rejection verified")

with tempfile.TemporaryDirectory() as directory:
    report = Path(directory) / "duplicate-key-report.json"
    report.write_text(
        '{"schema":"beatglass-native-host-provenance-v1","valid":true,'
        '"platform":"linux","name":"first","name":"second",'
        '"checked_fields":["name","platform"]}',
        encoding="utf-8",
    )
    result = subprocess.run(
        ["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "report is ambiguous" in result.stderr
print("Native host provenance duplicate-key rejection verified")
