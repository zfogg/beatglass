from pathlib import Path
import json
import subprocess
import tempfile

source = Path("tests/pipewire/validate_native_host_provenance_report.py").read_text()
assert "host provenance report has an unsupported schema" in source
assert "host provenance report has invalid checked fields" in source
assert "host provenance report omits required provenance fields" in source
print("Native host provenance report validator verified")

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
