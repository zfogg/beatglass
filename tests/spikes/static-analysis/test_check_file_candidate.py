import hashlib
from pathlib import Path
import tempfile
import unittest
from check_file_candidate import matches

class CandidateTests(unittest.TestCase):
    def test_matching_and_modified_files(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.exe"
            path.write_bytes(b"original")
            expected = hashlib.sha256(b"original").hexdigest()
            self.assertTrue(matches(path, expected))
            path.write_bytes(b"modified")
            self.assertFalse(matches(path, expected))
            path.unlink()
            with self.assertRaises(OSError):
                matches(path, expected)
    def test_bad_digest_is_rejected(self):
        with self.assertRaises(ValueError):
            matches("unused", "not-a-digest")

if __name__ == "__main__":
    unittest.main()
