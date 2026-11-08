from pathlib import Path
import unittest


class AnalysisJobBridgeContractTest(unittest.TestCase):
    def test_retry_paths_refresh_gui_state_immediately(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('emit progressChanged(); emit statusChanged(); poll();', source)
        self.assertIn('if (accepted) { poll_timer_.start(); emit statusChanged(); poll(); }', source)


if __name__ == "__main__":
    unittest.main()
