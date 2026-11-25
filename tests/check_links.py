"""Fail when a relative Markdown link in the root or docs/ points at a missing file."""
import pathlib
import re
import sys

files = list(pathlib.Path(".").glob("*.md")) + list(pathlib.Path("docs").glob("*.md"))
bad = [
    (str(f), link)
    for f in files
    for link in re.findall(r"\]\(((?!https?:)[^)#]+)", f.read_text(encoding="utf-8"))
    if not (f.parent / link).exists()
]
print("broken links:", bad) if bad else print("links ok")
sys.exit(1 if bad else 0)
