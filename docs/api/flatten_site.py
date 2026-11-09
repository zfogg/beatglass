"""Copy the generated API section to the Pages artifact root."""
from pathlib import Path
import shutil
import sys


output = Path(sys.argv[1])
api = output / "docs" / "api"
if not api.is_dir():
    raise SystemExit(f"missing generated API directory: {api}")

for source in api.iterdir():
    destination = output / source.name
    if source.is_dir():
        shutil.copytree(source, destination, dirs_exist_ok=True)
    else:
        shutil.copy2(source, destination)

for page in output.glob("*.html"):
    text = page.read_text(encoding="utf-8")
    text = text.replace('href="../../_static/', 'href="_static/')
    text = text.replace('src="../../_static/', 'src="_static/')
    text = text.replace('href="../design/', 'href="design/')
    text = text.replace('href="../../design/', 'href="design/')
    text = text.replace('href="../../tests/', 'href="tests/')
    text = text.replace('href="../../skills/', 'href="skills/')
    text = text.replace('href="../../AGENTS.html"', 'href="AGENTS.html"')
    text = text.replace('href="../', 'href="docs/')
    page.write_text(text, encoding="utf-8")
