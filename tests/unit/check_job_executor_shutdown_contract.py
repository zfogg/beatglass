from pathlib import Path

source = Path("include/beatglass/library/detail/job_executor.hpp").read_text()
for phrase in (
    "Request shutdown, discard queued jobs, and join the worker",
    "request active cancellation",
    "never from a job",
):
    if phrase not in source:
        raise SystemExit(f"missing executor shutdown contract: {phrase}")
print("job executor shutdown contract verified")
