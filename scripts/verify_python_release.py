"""Verify tested GitHub release assets before uv publishes them to a registry."""

import argparse
import ast
import configparser
import hashlib
import json
import re
import tarfile
import urllib.error
import urllib.request
import zipfile
from email.parser import BytesParser
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def python_version(tag):
    match = re.fullmatch(r"v(\d+\.\d+\.\d+)(?:-(alpha|beta|rc)\.(\d+))?", tag)
    require(match is not None, "Use a release tag such as v0.1.0-beta.1 or v0.1.0")
    base, stage, number = match.groups()
    return base + ({"alpha": "a", "beta": "b", "rc": "rc"}[stage] + number if stage else "")


def check_metadata(data, version):
    metadata = BytesParser().parsebytes(data)
    for field, expected in (
        ("Name", "serial-ledger"), ("Version", version),
        ("License-Expression", "MIT"), ("Requires-Python", ">=3.11"),
    ):
        require(metadata.get_all(field) == [expected], f"Unexpected {field} metadata")
    require("LICENSE.md" in metadata.get_all("License-File", []), "Missing license metadata")
    dependencies = metadata.get_all("Requires-Dist", [])
    for package in ("pyserial", "openpyxl"):
        require(any(item.startswith(package) for item in dependencies),
                f"Missing dependency: {package}")


def verify_assets(directory, version):
    directory = Path(directory)
    wheel_name = f"serial_ledger-{version}-py3-none-any.whl"
    sdist_name = f"serial_ledger-{version}.tar.gz"
    checksums = {}
    for line in (directory / "SHA256SUMS.txt").read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"([a-f0-9]{64})  ([A-Za-z0-9_.-]+)", line)
        require(match is not None, "Invalid checksum manifest line")
        digest, name = match.groups()
        require(name not in checksums, f"Duplicate checksum entry: {name}")
        checksums[name] = digest
    digests = {}
    for name in (wheel_name, sdist_name):
        digest = hashlib.sha256((directory / name).read_bytes()).hexdigest()
        require(checksums.get(name) == digest, f"Checksum mismatch: {name}")
        digests[name] = digest
    with zipfile.ZipFile(directory / wheel_name) as archive:
        prefix = f"serial_ledger-{version}.dist-info"
        check_metadata(archive.read(f"{prefix}/METADATA"), version)
        license_text = archive.read(f"{prefix}/licenses/LICENSE.md")
        require(license_text.startswith(b"MIT License"), "Missing MIT license text")
        entry_points = configparser.ConfigParser()
        entry_points.read_string(archive.read(f"{prefix}/entry_points.txt").decode("utf-8"))
        require(entry_points["console_scripts"]["serial-ledger"] == "serial_ledger.cli:main",
                "Wrong CLI entry point")
        module = ast.parse(archive.read("serial_ledger/__init__.py"))
        versions = [
            ast.literal_eval(node.value) for node in module.body
            if isinstance(node, ast.Assign)
            and any(isinstance(target, ast.Name) and target.id == "__version__"
                    for target in node.targets)
        ]
        require(versions == [version], "Wheel runtime version differs from the release tag")
    with tarfile.open(directory / sdist_name) as archive:
        prefix = f"serial_ledger-{version}"
        check_metadata(archive.extractfile(f"{prefix}/PKG-INFO").read(), version)
        require(archive.extractfile(f"{prefix}/LICENSE.md").read() == license_text,
                "Wheel and source licenses differ")
    return digests


def require_testpypi(version, digests):
    url = f"https://test.pypi.org/pypi/serial-ledger/{version}/json"
    try:
        with urllib.request.urlopen(url, timeout=20) as response:
            data = json.load(response)
    except urllib.error.HTTPError as error:
        if error.code == 404:
            raise ValueError("Publish this version to TestPyPI before PyPI") from error
        raise
    require(data["info"]["version"] == version, "Wrong TestPyPI version")
    uploaded = {item["filename"]: item for item in data["urls"]}
    for name, digest in digests.items():
        item = uploaded.get(name)
        require(item is not None, f"Missing TestPyPI file: {name}")
        require(not item.get("yanked"), f"TestPyPI file was yanked: {name}")
        require(item["digests"]["sha256"] == digest, f"TestPyPI file differs: {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--directory", type=Path, default=Path("dist"))
    parser.add_argument("--print-version", action="store_true")
    parser.add_argument("--require-testpypi", action="store_true")
    args = parser.parse_args()
    version = python_version(args.tag)
    if args.print_version:
        print(version)
        return
    digests = verify_assets(args.directory, version)
    if args.require_testpypi:
        require_testpypi(version, digests)
    print(f"Verified serial-ledger {version}: "
          "wheel, source archive, checksums, and CLI entry point")


if __name__ == "__main__":
    main()
