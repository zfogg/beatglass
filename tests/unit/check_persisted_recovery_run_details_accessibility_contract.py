"""Contract checks for persisted recovery diagnostic details accessibility."""

from pathlib import Path


QML = Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml"


def main() -> None:
    source = QML.read_text(encoding="utf-8")
    required = [
        'objectName: "persistedRecoveryRunDetails"',
        'Accessible.name: "Persisted retryable analysis identities"',
        'Accessible.value: visible ? text : ""',
        'Accessible.description: visible && typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""',
        '"Diagnostic details for selected persisted recovery run " + storageRecovery.selectedRunId',
        '"Run ID, content identity, source path, and decoder snapshot for each saved retryable analysis."',
        'storageRecovery.retryableRunDetails.join("\\n")',
    ]
    missing = [marker for marker in required if marker not in source]
    if missing:
        raise AssertionError("missing persisted recovery details accessibility markers: " + ", ".join(missing))
    print(f"persisted recovery run details accessibility contract: {len(required)} checks passed")


if __name__ == "__main__":
    main()
