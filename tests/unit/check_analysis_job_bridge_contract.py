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

    def test_fully_rejected_batch_publishes_failed_state(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('else if (batchRejected_ != 0 && status_ != QStringLiteral("Failed"))', source)

    def test_cancellation_clears_stale_results(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertGreaterEqual(source.count('if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }'), 3)

    def test_cancellation_api_documents_clear_behavior(self):
        header = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
        self.assertIn('publish Cancelled immediately, and clear stale result presentation.', header)
        self.assertIn('publish Cancelled immediately, and clear stale result/batch presentation.', header)
    def test_cancellation_publishes_terminal_state_immediately(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertGreaterEqual(source.count('status_ = QStringLiteral("Cancelled"); emit statusChanged();'), 2)

    def test_batch_cancellation_clears_failure_summary(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0; batchLastResult_.clear();', source)
        self.assertIn('emit batchChanged();', source)

    def test_batch_submission_clears_previous_failure_diagnostic(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('total_frames_ = 0; progress_known_ = false; emit progressChanged(); }\n  if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }', source)

    def test_single_submission_clears_batch_aggregates(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('batchAccepted_ = batchRejected_ = batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0;', source)

    def test_single_submission_cancels_active_batch_operations(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('for (const auto& operation : batch_) if (operation) operation->cancel();\n  batch_.clear();', source)

    def test_batch_submission_cancels_active_single_operation(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('if (retryable_ && canCancel()) retryable_->cancel();\n  retryable_.reset();', source)

    def test_batch_submission_retires_single_job_handle(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('retryable_.reset();', source)

    def test_batch_cancellation_resets_all_terminal_counters(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0; batchLastResult_.clear();', source)

    def test_cancellation_clears_failure_diagnostic(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertGreaterEqual(source.count('if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }'), 3)

    def test_cancellation_clears_failure_diagnostic(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertGreaterEqual(source.count('if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }'), 3)

    def test_cancellation_clears_failure_diagnostic(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertGreaterEqual(source.count('if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }'), 3)

    def test_cancellation_clears_failure_diagnostic(self):
        source = (Path(__file__).parents[2] / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
        self.assertGreaterEqual(source.count('if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }'), 3)


if __name__ == "__main__":
    unittest.main()
