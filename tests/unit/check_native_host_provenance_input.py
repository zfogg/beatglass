import json
import subprocess
import tempfile
from pathlib import Path

with tempfile.TemporaryDirectory() as directory:
    for arguments in ([], ["one.json", "two.json", "three.json"]):
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", *arguments], capture_output=True, text=True)
        assert result.returncode != 0
        assert "usage: validate_native_host_provenance.py HOST.json [REPORT.json]" in result.stderr

    missing = Path(directory) / "missing.json"
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(missing)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance is unreadable" in result.stderr
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", directory], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance is unreadable" in result.stderr

    for value in ("null", "[]", "42"):
        scalar = Path(directory) / "scalar.json"
        scalar.write_text(value, encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(scalar)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance must be an object" in result.stderr

    boolean_id = Path(directory) / "boolean-node.json"
    boolean_id.write_text('{"platform":"linux","name":"qualification-host","node_id":true}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(boolean_id)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance has an invalid node id" in result.stderr
    for name in ("", "   ", "\t\n"):
        blank_name = Path(directory) / "blank-name.json"
        blank_name.write_text(json.dumps({"platform": "linux", "name": name}), encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(blank_name)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance has no name" in result.stderr
    for platform in ("android", "", "LINUX"):
        invalid_platform = Path(directory) / "invalid-platform.json"
        invalid_platform.write_text(json.dumps({"platform": platform, "name": "qualification-host"}), encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(invalid_platform)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance has an invalid platform" in result.stderr
    fractional_node = Path(directory) / "fractional-node.json"
    fractional_node.write_text('{"platform":"linux","name":"qualification-host","node_id":1.5}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(fractional_node)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance has an invalid node id" in result.stderr

    invalid_secure_boot = Path(directory) / "invalid-secure-boot.json"
    invalid_secure_boot.write_text('{"platform":"linux","name":"qualification-host","secure_boot":1}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(invalid_secure_boot)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance has an invalid secure boot flag" in result.stderr

    invalid_tpm = Path(directory) / "invalid-tpm.json"
    invalid_tpm.write_text('{"platform":"linux","name":"qualification-host","tpm_present":0}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(invalid_tpm)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance has an invalid TPM flag" in result.stderr

    for node_id in (0, -7):
        invalid_node = Path(directory) / "invalid-node.json"
        invalid_node.write_text(f'{{"platform":"linux","name":"qualification-host","node_id":{node_id}}}', encoding="utf-8")
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(invalid_node)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance has an invalid node id" in result.stderr
    string_node = Path(directory) / "string-node.json"
    string_node.write_text('{"platform":"linux","name":"qualification-host","node_id":"7"}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(string_node)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance has an invalid node id" in result.stderr

    malformed = Path(directory) / "malformed.json"
    malformed.write_text("{not-json", encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(malformed)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance is unreadable" in result.stderr

    duplicate = Path(directory) / "duplicate.json"
    duplicate.write_text('{"platform":"linux","name":"one","name":"two"}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(duplicate)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance is ambiguous" in result.stderr

    host = Path(directory) / "host.json"
    host.write_text('{"platform":"linux","name":"qualification-host"}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), directory], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report is unwritable" in result.stderr
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(host)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report is unwritable" in result.stderr
    assert json.loads(host.read_text(encoding="utf-8"))["name"] == "qualification-host"
    hardlink = Path(directory) / "host-hardlink.json"
    try:
        hardlink.hardlink_to(host)
    except (OSError, NotImplementedError):
        pass
    else:
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(hardlink)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance report is unwritable" in result.stderr
    relative_self = Path(directory) / "nested" / ".." / "host.json"
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(relative_self)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report is unwritable" in result.stderr
    assert not list(Path(directory).glob("tmp*"))

    collision = Path(directory) / "not-a-directory"
    collision.write_text("occupied", encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(collision / "report.json")], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report is unwritable" in result.stderr

    symlink_target = Path(directory) / "target.json"
    symlink_target.write_text("untouched", encoding="utf-8")
    symlink = Path(directory) / "link.json"
    try:
        symlink.symlink_to(symlink_target)
    except (OSError, NotImplementedError):
        pass
    else:
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(symlink)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance report is unwritable" in result.stderr
        assert symlink_target.read_text(encoding="utf-8") == "untouched"
    host_link = Path(directory) / "host-link.json"
    try:
        host_link.symlink_to(host)
    except (OSError, NotImplementedError):
        pass
    else:
        result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host_link)], capture_output=True, text=True)
        assert result.returncode != 0
        assert "host provenance is unreadable" in result.stderr
print("Native host provenance input rejection verified")
