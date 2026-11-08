from pathlib import Path
import unittest


class AnalysisJobBridgeContractTest(unittest.TestCase):
    def test_retry_paths_refresh_gui_state_immediately(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('status_ = QStringLiteral("Queued"); poll_timer_.start(); emit statusChanged(); poll();', source)
        self.assertIn('poll_timer_.start(); emit statusChanged(); poll();', source)

    def test_batch_submission_publishes_queued_immediately(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('if (status_ != QStringLiteral("Queued")) { status_ = QStringLiteral("Queued"); emit statusChanged(); }', source)


if __name__ == "__main__":
    unittest.main()
