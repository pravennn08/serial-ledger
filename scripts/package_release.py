"""Build and validate local beta assets; never publish or create Git tags."""

import ast
import hashlib
import json
import re
import subprocess
import tarfile
import tomllib
import zipfile
from email.parser import BytesParser
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def properties(path):
    return dict(
        line.split("=", 1)
        for line in path.read_text(encoding="utf-8").splitlines()
        if line and not line.startswith("#")
    )


def validate_metadata(data, project):
    metadata = BytesParser().parsebytes(data)
    require(metadata["Name"] == project["name"], "Wrong package name")
    require(metadata["Version"] == project["version"], "Wrong Python package version")
    require(metadata["License-Expression"] == "MIT", "Missing SPDX MIT license")
    require("LICENSE.md" in metadata.get_all("License-File", []), "Missing license metadata")
    require(metadata["Requires-Python"] == ">=3.11", "Wrong Python requirement")
    author = project["authors"][0]
    require(author["email"] in metadata["Author-email"], "Missing author email")
    dependencies = metadata.get_all("Requires-Dist", [])
    for name in ("pyserial", "openpyxl"):
        require(any(item.startswith(name) for item in dependencies), f"Missing {name} dependency")


def main():
    project = tomllib.loads((ROOT / "pyproject.toml").read_text(encoding="utf-8"))["project"]
    version = project["version"]
    match = re.fullmatch(r"(\d+\.\d+\.\d+)b(\d+)", version)
    require(match is not None, "Expected a Python beta version such as 0.1.0b1")
    base, beta = match.groups()
    board_version = f"{base}-beta.{beta}"
    library = ROOT / "library"
    props = properties(library / "library.properties")
    manifest = json.loads((library / "library.json").read_text(encoding="utf-8"))
    require(props["version"] == board_version == manifest["version"], "Board version mismatch")
    require(manifest["license"] == project["license"] == "MIT", "License metadata mismatch")
    require(props["url"] == project["urls"]["Homepage"], "Project URL mismatch")
    maintainer = project["maintainers"][0]
    require(props["maintainer"] == f'{maintainer["name"]} <{maintainer["email"]}>',
            "Arduino maintainer mismatch")
    module = ast.parse((ROOT / "desktop/src/serial_ledger/__init__.py").read_text())
    runtime_version = next(
        ast.literal_eval(node.value) for node in module.body
        if isinstance(node, ast.Assign)
        and any(isinstance(target, ast.Name) and target.id == "__version__"
                for target in node.targets)
    )
    require(runtime_version == version, "Python runtime version mismatch")
    header = (library / "src/serialledger_c.h").read_text()
    require(f'#define SL_VERSION "{board_version}"' in header, "C runtime version mismatch")
    for path in (ROOT / "CMakeLists.txt", library / "CMakeLists.txt"):
        require(f"VERSION {base} " in path.read_text(), "CMake numeric version mismatch")
    license_text = (ROOT / "LICENSE.md").read_bytes()
    require((library / "LICENSE.md").read_bytes() == license_text, "Board license copy differs")

    # Separate builds also work around Windows launcher path canonicalization issues.
    subprocess.run(["uv", "build", "--wheel"], cwd=ROOT, check=True)
    subprocess.run(["uv", "build", "--sdist"], cwd=ROOT, check=True)
    dist = ROOT / "dist"
    wheel = dist / f"serial_ledger-{version}-py3-none-any.whl"
    sdist = dist / f"serial_ledger-{version}.tar.gz"
    with zipfile.ZipFile(wheel) as archive:
        prefix = f"serial_ledger-{version}.dist-info"
        validate_metadata(archive.read(f"{prefix}/METADATA"), project)
        require(archive.read(f"{prefix}/licenses/LICENSE.md") == license_text,
                "Wheel license differs")
    with tarfile.open(sdist) as archive:
        prefix = f"serial_ledger-{version}"
        validate_metadata(archive.extractfile(f"{prefix}/PKG-INFO").read(), project)
        require(archive.extractfile(f"{prefix}/LICENSE.md").read() == license_text,
                "Source distribution license differs")

    board_zip = dist / f"SerialLedger-{board_version}.zip"
    files = [library / name for name in (
        "CMakeLists.txt", "library.properties", "library.json", "README.md",
        "CHANGELOG.md", "LICENSE.md"
    )]
    for folder in ("src", "examples"):
        files.extend(sorted(path for path in (library / folder).rglob("*") if path.is_file()))
    with zipfile.ZipFile(board_zip, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for path in files:
            archive.write(path, f"SerialLedger/{path.relative_to(library).as_posix()}")
        archive.write(ROOT / "docs/BOARD_API.md", "SerialLedger/docs/BOARD_API.md")
    with zipfile.ZipFile(board_zip) as archive:
        require(archive.read("SerialLedger/LICENSE.md") == license_text, "ZIP license differs")
        for name in ("src/SerialLedger.h", "src/serialledger_c.h", "src/reading.c",
                     "src/serialledger.c", "examples/BasicTelemetry/BasicTelemetry.ino"):
            require(f"SerialLedger/{name}" in archive.namelist(), f"Missing {name} in ZIP")
    assets = (wheel, sdist, board_zip)
    checksums = "".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n" for path in assets
    )
    (dist / "SHA256SUMS.txt").write_text(checksums, encoding="utf-8", newline="\n")
    for path in (*assets, dist / "SHA256SUMS.txt"):
        print(f"Prepared {path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
