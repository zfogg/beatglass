import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("signoff", Path(__file__).with_name("record_release_signoff.py"))
signoff = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(signoff)


class ReleaseSignoffTest(unittest.TestCase):
    def test_pending_decision_cannot_receive_signoff(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); decision = root / "decision.json"; output = root / "signoff.json"
            decision.write_text(json.dumps({"release_decision_schema": 1, "decision": "PENDING"}), encoding="utf-8")
            self.assertIn("non-ACCEPTED", signoff.signoff(decision, "release-owner", output)[0])
            self.assertFalse(output.exists())

    def test_invalid_reviewer_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); decision = root / "decision.json"
            decision.write_text(json.dumps({"release_decision_schema": 1, "decision": "ACCEPTED", "report_sha256": "a" * 64}), encoding="utf-8")
            self.assertIn("reviewer identity", signoff.signoff(decision, " ", root / "signoff.json")[0])
