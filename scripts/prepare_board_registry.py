"""Export a standalone board-library repository without changing release assets."""

import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def prepare(destination, repository):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9-]*/[A-Za-z0-9][A-Za-z0-9_.-]*", repository):
        raise ValueError("Repository must be a GitHub owner/name, without a URL")
    library = ROOT / "library"
    props = dict(
        line.split("=", 1) for line in (library / "library.properties").read_text().splitlines()
        if line and not line.startswith("#")
    )
    manifest = json.loads((library / "library.json").read_text())
    version = props["version"]
    if version != manifest["version"]:
        raise ValueError("Arduino and PlatformIO library versions differ")
    if f'#define SL_VERSION "{version}"' not in (library / "src/serialledger_c.h").read_text():
        raise ValueError("Library runtime version differs from its manifests")
    if (library / "LICENSE.md").read_bytes() != (ROOT / "LICENSE.md").read_bytes():
        raise ValueError("Board library license differs from the project license")

    # Never overwrite an existing export or repository: it may contain user work.
    destination.mkdir(parents=True, exist_ok=False)
    for name in ("src", "examples"):
        shutil.copytree(library / name, destination / name)
    for name in ("CMakeLists.txt", "README.md", "CHANGELOG.md", "LICENSE.md"):
        shutil.copyfile(library / name, destination / name)
    (destination / "docs").mkdir()
    shutil.copyfile(ROOT / "docs/BOARD_API.md", destination / "docs/BOARD_API.md")
    url = f"https://github.com/{repository}"
    props["url"] = url
    (destination / "library.properties").write_text(
        "".join(f"{key}={value}\n" for key, value in props.items()),
        encoding="utf-8", newline="\n",
    )
    manifest["homepage"] = url
    manifest["repository"]["url"] = f"{url}.git"
    # Keep repository automation, build products, and VCS metadata out of packages.
    manifest["export"] = {"include": [
        "src", "examples", "docs", "CMakeLists.txt", "library.properties",
        "library.json", "README.md", "CHANGELOG.md", "LICENSE.md", "UPSTREAM.md",
    ]}
    (destination / "library.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n",
    )
    (destination / ".gitignore").write_text(
        "build/\ndist/\n.pio/\n", encoding="utf-8", newline="\n",
    )
    workflow = destination / ".github/workflows/ci.yml"
    workflow.parent.mkdir(parents=True)
    shutil.copyfile(ROOT / "scripts/board-registry/ci.yml", workflow)
    commit = subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, check=True, capture_output=True, text=True,
    ).stdout.strip()
    (destination / "UPSTREAM.md").write_text(
        "# Source and maintenance\n\n"
        "This repository distributes the SerialLedger C99/C++11 board library.\n"
        "Development and the Python CLI are in "
        "<https://github.com/pravennn08/serial-ledger>.\n\n"
        f"Exported from the development working tree based on commit `{commit}`.\n"
        "The export preserves source and example files, updates repository URLs,\n"
        "and adds the API guide and registry validation workflow.\n\n"
        "Make API changes in the development repository first, then export to a\n"
        "fresh directory and review the changes before updating this repository.\n"
        "Use a new library version for each subsequent registry release.\n",
        encoding="utf-8", newline="\n",
    )
    return version


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", default="pravennn08/SerialLedger")
    parser.add_argument("--output", type=Path, default=ROOT / "work/board-registry/SerialLedger")
    args = parser.parse_args()
    version = prepare(args.output, args.repository)
    print(f"Prepared SerialLedger {version} at {args.output.resolve()}")
    print("No Git repository, tag, release, or registry submission has been created.")


if __name__ == "__main__":
    main()
