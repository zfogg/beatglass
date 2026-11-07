from pathlib import Path
import shlex
import subprocess

project = "Beatglass API"
extensions = ["hawkmoth"]
hawkmoth_root = str(Path(__file__).resolve().parents[2])
hawkmoth_compiler = "clang++-18"
hawkmoth_clang_cpp = ["-std=c++20", "-I" + str(Path(hawkmoth_root) / "include")]
hawkmoth_clang_cpp += shlex.split(subprocess.check_output(["pkg-config", "--cflags", "Qt6Core"], text=True))
html_theme = "alabaster"
