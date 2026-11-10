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

    invalid_secure_boot = Path(directory) / "invalid-secure-boot.json"
    invalid_secure_boot.write_text('{"platform":"linux","name":"qualification-host","secure_boot":1}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(invalid_secure_boot)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance has an invalid secure boot flag" in result.stderr

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
print("Native host provenance input rejection verified")
