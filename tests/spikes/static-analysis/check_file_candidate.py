"""Offline candidate-file digest check; never establishes running-image identity."""
import hashlib
import hmac
import re
import sys


def matches(path, expected):
    if not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise ValueError("expected digest must be 64 lowercase hex characters")
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        while chunk := source.read(1024 * 1024):
            digest.update(chunk)
    return hmac.compare_digest(digest.hexdigest(), expected)


def main():
    try:
        valid = matches(sys.argv[1], sys.argv[2])
    except (IndexError, OSError, ValueError) as error:
        print(f"candidate check failed: {error}", file=sys.stderr)
        return 2
    if not valid:
        print("candidate digest mismatch", file=sys.stderr)
        return 1
    print("candidate file matches supplied digest; loaded image is not verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
