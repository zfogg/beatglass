import unittest
from check_imports import validate

class ImportChecks(unittest.TestCase):
    def test_system_only_is_accepted(self):
        self.assertEqual(validate("DLL Name: KERNEL32.dll\nDLL Name: msvcrt.dll\n"), {"kernel32.dll", "msvcrt.dll"})
    def test_unexpected_or_missing_are_rejected(self):
        with self.assertRaises(ValueError):
            validate("DLL Name: KERNEL32.dll\nDLL Name: libaubio-5.dll\n")
        with self.assertRaises(ValueError):
            validate("no import records")

if __name__ == "__main__":
    unittest.main()
