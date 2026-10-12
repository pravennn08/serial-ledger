"""Publishing preflight rejects altered assets and untested production uploads."""

import hashlib
import importlib.util
import io
import json
import tarfile
import tempfile
import unittest
import urllib.error
import zipfile
from pathlib import Path
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location(
    "verify_python_release",
    Path(__file__).resolve().parents[2] / "scripts/verify_python_release.py",
)
publisher = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(publisher)


class PublishTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        self.version = "0.1.0b1"
        self.wheel = self.directory / f"serial_ledger-{self.version}-py3-none-any.whl"
        self.sdist = self.directory / f"serial_ledger-{self.version}.tar.gz"
        metadata = (
            "Metadata-Version: 2.4\nName: serial-ledger\nVersion: 0.1.0b1\n"
            "License-Expression: MIT\nLicense-File: LICENSE.md\nRequires-Python: >=3.11\n"
            "Requires-Dist: pyserial<4,>=3.5\nRequires-Dist: openpyxl<4,>=3.1\n\n"
        ).encode()
        license_text = b"MIT License\nFixture license for packaging tests.\n"
        with zipfile.ZipFile(self.wheel, "w") as archive:
            prefix = f"serial_ledger-{self.version}.dist-info"
            archive.writestr(f"{prefix}/METADATA", metadata)
            archive.writestr(f"{prefix}/licenses/LICENSE.md", license_text)
            archive.writestr(f"{prefix}/entry_points.txt",
                             "[console_scripts]\nserial-ledger = serial_ledger.cli:main\n")
            archive.writestr("serial_ledger/__init__.py", '__version__ = "0.1.0b1"\n')
        with tarfile.open(self.sdist, "w:gz") as archive:
            prefix = f"serial_ledger-{self.version}"
            for name, data in (("PKG-INFO", metadata), ("LICENSE.md", license_text)):
                info = tarfile.TarInfo(f"{prefix}/{name}")
                info.size = len(data)
                archive.addfile(info, io.BytesIO(data))
        checksums = "".join(
            f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
            for path in (self.wheel, self.sdist)
        )
        (self.directory / "SHA256SUMS.txt").write_text(checksums, encoding="utf-8")

    def test_release_tags_map_to_python_versions_without_accepting_arbitrary_input(self):
        self.assertEqual(publisher.python_version("v0.1.0-beta.1"), "0.1.0b1")
        self.assertEqual(publisher.python_version("v0.1.0"), "0.1.0")
        for tag in ("main", "../../file", "v0.1.0-beta.1\nmalicious", "--help"):
            with self.subTest(tag=tag), self.assertRaises(ValueError):
                publisher.python_version(tag)

    def test_valid_release_assets_pass_but_altered_wheel_is_rejected(self):
        digests = publisher.verify_assets(self.directory, self.version)
        self.assertEqual(set(digests), {self.wheel.name, self.sdist.name})
        with self.wheel.open("ab") as stream:
            stream.write(b"modified after testing")
        with self.assertRaisesRegex(ValueError, "Checksum mismatch"):
            publisher.verify_assets(self.directory, self.version)

    def test_metadata_version_must_match_the_release_tag(self):
        data = (
            b"Name: serial-ledger\nVersion: 0.1.0\nLicense-Expression: MIT\n"
            b"Requires-Python: >=3.11\n"
        )
        with self.assertRaisesRegex(ValueError, "Version"):
            publisher.check_metadata(data, self.version)

    def test_production_requires_identical_unyanked_testpypi_assets(self):
        digests = publisher.verify_assets(self.directory, self.version)
        data = {"info": {"version": self.version}, "urls": [
            {"filename": name, "digests": {"sha256": digest}, "yanked": False}
            for name, digest in digests.items()
        ]}
        with patch.object(publisher.urllib.request, "urlopen",
                          return_value=io.BytesIO(json.dumps(data).encode())):
            publisher.require_testpypi(self.version, digests)
        data["urls"][0]["digests"]["sha256"] = "0" * 64
        with patch.object(publisher.urllib.request, "urlopen",
                          return_value=io.BytesIO(json.dumps(data).encode())):
            with self.assertRaisesRegex(ValueError, "differs"):
                publisher.require_testpypi(self.version, digests)
        data["urls"][0]["digests"]["sha256"] = digests[data["urls"][0]["filename"]]
        data["urls"][0]["yanked"] = True
        with patch.object(publisher.urllib.request, "urlopen",
                          return_value=io.BytesIO(json.dumps(data).encode())):
            with self.assertRaisesRegex(ValueError, "yanked"):
                publisher.require_testpypi(self.version, digests)

    def test_missing_testpypi_release_blocks_production(self):
        error = urllib.error.HTTPError("https://test.pypi.org/", 404, "Not Found", {}, None)
        with patch.object(publisher.urllib.request, "urlopen", side_effect=error):
            with self.assertRaisesRegex(ValueError, "TestPyPI before PyPI"):
                publisher.require_testpypi(self.version, {})


if __name__ == "__main__":
    unittest.main()
