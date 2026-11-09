"""Source contract for initial Windows output endpoint selection."""

from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "src/desktop/platform/windows_endpoint_bridge.cpp").read_text(encoding="utf-8")


def main() -> None:
    assert "previous.isEmpty() && selected_.isEmpty()" in SOURCE
    assert "endpoint.is_default" in SOURCE
    assert "select_render_endpoint(snapshot_, endpoint.id)" in SOURCE
    assert "selected_ = QString::fromStdString(endpoint.id)" in SOURCE
    print("Windows endpoint default-selection contract verified (4 checks)")


if __name__ == "__main__":
    main()
