import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("consumer", Path(__file__).with_name("consume_review_packet.py"))
consumer = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(consumer)


class ReviewPacketConsumerTest(unittest.TestCase):
    def test_pending_packet_is_rejected(self):
        report = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        review_spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
        review = importlib.util.module_from_spec(review_spec); assert review_spec.loader
        review_spec.loader.exec_module(review)
        with tempfile.TemporaryDirectory() as tmp:
            packet_path = Path(tmp) / "review.json"; packet_path.write_text(json.dumps(review.review_packet(report)), encoding="utf-8")
            self.assertTrue(any("acceptance errors" in error or "not accepted" in error for error in consumer.consume(packet_path)))

    def test_unknown_packet_schema_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "review.json"; path.write_text(json.dumps({"review_packet_schema": 99}), encoding="utf-8")
            self.assertTrue(consumer.consume(path))

    def test_packet_without_report_hash_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "review.json"; path.write_text(json.dumps({"review_packet_schema": 1}), encoding="utf-8")
            self.assertTrue(any("invalid report hash anchor" in error for error in consumer.consume(path)))

    def test_tampered_ready_packet_is_rejected(self):
        report = Path(__file__).parent / "fixtures/readiness-bundles/readiness-report.json"
        review_spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
        review = importlib.util.module_from_spec(review_spec); assert review_spec.loader
        review_spec.loader.exec_module(review)
        packet = review.review_packet(report); packet["gate_status"] = "READY"; packet["acceptance_errors"] = []
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "review.json"; path.write_text(json.dumps(packet), encoding="utf-8")
            self.assertTrue(any("requires native qualification" in error for error in consumer.consume(path)))
