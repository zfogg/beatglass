import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("workflow", Path(__file__).with_name("prepare_release_bundle.py"))
workflow = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(workflow)


class ReleaseWorkflowTest(unittest.TestCase):
    def test_pending_native_evidence_stops_before_release(self):
        fixture = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); packet = root / "review.json"
            packet.write_text("{}", encoding="utf-8")
            retained = root / "retained"; release = root / "release"
            errors = workflow.prepare(fixture, packet, retained, release)
            self.assertTrue(errors)
            self.assertFalse(release.exists())
