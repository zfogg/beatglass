from pathlib import Path
from datetime import datetime
import shlex
import subprocess

project = "Beatglass API"
master_doc = "index"
extensions = ["hawkmoth", "hawkmoth.ext.javadoc", "myst_parser"]
hawkmoth_root = str(Path(__file__).resolve().parents[2])
hawkmoth_compiler = "clang++-18"
hawkmoth_clang_cpp = ["-std=c++20", "-I" + str(Path(hawkmoth_root) / "include")]
hawkmoth_clang_cpp += shlex.split(subprocess.check_output(["pkg-config", "--cflags", "Qt6Core"], text=True))
html_theme = "alabaster"
copyright = f"Zachary Fogg {datetime.now().year}"
templates_path = ["_templates"]
html_static_path = ["_static"]
html_css_files = ["custom.css"]
html_js_files = ["signature_tokens.js"]
html_sidebars = {"**": ["searchbox.html", "docsnav.html", "localtoc.html"]}
exclude_patterns = [".worktree-*", "build", ".git", "tests/fixtures/generated"]

hawkmoth_transform_default = "javadoc"
myst_enable_extensions = ["colon_fence", "deflist", "fieldlist", "substitution"]
suppress_warnings = ["toc.not_included", "myst.xref_missing", "cpp", "cpp.*", "*"]
