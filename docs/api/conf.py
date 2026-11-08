from pathlib import Path
import shlex
import subprocess

project = "Beatglass API"
master_doc = "docs/api/index"
extensions = ["hawkmoth", "hawkmoth.ext.javadoc", "myst_parser"]
hawkmoth_root = str(Path(__file__).resolve().parents[2])
hawkmoth_compiler = "clang++-18"
hawkmoth_clang_cpp = ["-std=c++20", "-I" + str(Path(hawkmoth_root) / "include")]
hawkmoth_clang_cpp += shlex.split(subprocess.check_output(["pkg-config", "--cflags", "Qt6Core"], text=True))
html_theme = "alabaster"
exclude_patterns = [".worktree-*", "build", ".git", "tests/fixtures/generated"]

hawkmoth_transform_default = "javadoc"
myst_enable_extensions = ["colon_fence", "deflist", "fieldlist", "substitution"]
