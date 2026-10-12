# Publish Python to TestPyPI, then PyPI

The Python project is named `serial-ledger`; its import name is `serial_ledger`.
The first beta version is `0.1.0b1`, matching GitHub tag `v0.1.0-beta.1`.
This process uses uv and GitHub Trusted Publishing without a stored API token.

## Accounts and environments

1. Create separate accounts at [TestPyPI](https://test.pypi.org/account/register/)
   and [PyPI](https://pypi.org/account/register/). Verify your email and configure
   two-factor authentication on each. Keep recovery codes privately.
2. Push `.github/workflows/publish-python.yml`, the verifier, and these docs to
   the repository's `main` branch. No release tag needs to be changed.
3. In GitHub **Settings > Environments**, create `testpypi` and `pypi`.
4. Configure a pending GitHub Trusted Publisher at
   [TestPyPI Publishing](https://test.pypi.org/manage/account/publishing/) and
   [PyPI Publishing](https://pypi.org/manage/account/publishing/) using these values:

| Field | TestPyPI | PyPI |
| --- | --- | --- |
| PyPI project name | `serial-ledger` | `serial-ledger` |
| GitHub repository owner | `pravennn08` | `pravennn08` |
| GitHub repository name | `serial-ledger` | `serial-ledger` |
| Workflow filename | `publish-python.yml` | `publish-python.yml` |
| Environment name | `testpypi` | `pypi` |

Check the owner against your actual GitHub repository URL; these values reflect
the current configured remote. The workflow filename excludes `.github/workflows/`.
A pending publisher creates the project on first upload; it does not reserve the
name. A 404 from the project JSON endpoint means no public project was found, but
PyPI may still reject reserved or prohibited names at upload time.

## Publish TestPyPI first

In GitHub **Actions > Publish Python > Run workflow**:

- Branch: `main`.
- Registry: `testpypi`.
- Release tag: `v0.1.0-beta.1`.

The workflow downloads the existing tested wheel, source archive, and checksums
from that GitHub release. It verifies versions, package metadata, MIT license,
entry point, and file hashes, then uploads only the two Python distributions.
It does not rebuild or upload the board ZIP, old `0.1.0` files, or checksum file.

The last step installs `serial-ledger==0.1.0b1` from TestPyPI in an isolated
environment and runs the CLI and loopback CSV/XLSX demo. Dependencies are installed
from normal PyPI first; the TestPyPI package is installed with `--no-deps`.
This avoids depending on copies of pySerial/openpyxl in the test registry.

Check the run is green and open
<https://test.pypi.org/project/serial-ledger/0.1.0b1/>.
If an upload succeeds but a later smoke test fails, fix the test/environment and
rerun; do not delete the published version or change files under the same version.

## Publish the same files to PyPI

Run the workflow again on `main`, with registry **`pypi`** and the same release tag.
The production step first requires the same wheel and source archive hashes to
exist on TestPyPI. It uploads those exact GitHub assets and tests installation
from production PyPI. Existing identical uploads are skipped using uv's check URL.

After a successful run, users can install the beta by name:

```sh
uv tool install --python 3.11 "serial-ledger==0.1.0b1"
serial-ledger --version
serial-ledger ports
serial-ledger demo
```

Account setup and publisher configuration must be completed before a real upload.
The workflow runs only when manually dispatched on `main`; normal CI does not publish.

## Local preflight

These commands verify files without uploading or requiring credentials:

```sh
uv run --locked python scripts/verify_python_release.py --tag v0.1.0-beta.1
uv publish --dry-run --trusted-publishing never --publish-url https://test.pypi.org/legacy/ dist/serial_ledger-0.1.0b1-py3-none-any.whl dist/serial_ledger-0.1.0b1.tar.gz
uv publish --dry-run --trusted-publishing never --publish-url https://upload.pypi.org/legacy/ dist/serial_ledger-0.1.0b1-py3-none-any.whl dist/serial_ledger-0.1.0b1.tar.gz
```

Always name the exact two files: `dist/` also contains board assets and older builds.
Do not rebuild the already-tested GitHub release assets during first publication.
For a new release, use a new version and tag with matching assets and checksums.

References: [uv publishing](https://docs.astral.sh/uv/guides/package/),
[PyPI pending publishers](https://docs.pypi.org/trusted-publishers/creating-a-project-through-oidc/),
[PyPI account requirements](https://pypi.org/help/#twofa).
