import subprocess
import sys
import tempfile
from pathlib import Path

producer = Path("tests/pipewire/validate_native_host_provenance.py")
with tempfile.TemporaryDirectory() as directory:
    host = Path(directory) / "host.json"
    host.write_text('{"platform":"linux","name":"host","tpm_present":true,"tpm_present":false}', encoding="utf-8")
    result = subprocess.run([sys.executable, str(producer), str(host)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "ambiguous" in result.stderr
print("Native host provenance duplicate TPM rejection verified")
