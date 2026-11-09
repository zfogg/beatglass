from pathlib import Path

source = Path("include/beatglass/library/job_lifecycle.hpp").read_text()
expected = "snapshot_.status = JobStatus::queued;\n    snapshot_.frames_processed = 0;"
if expected not in source:
    raise SystemExit("job retry must clear partial progress before requeue")
print("job retry progress reset contract verified")
