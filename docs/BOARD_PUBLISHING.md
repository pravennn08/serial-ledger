# Publish the C/C++ board library

The Python CLI is already on PyPI. The board library is a separate package:
**SerialLedger**, version **0.1.0-beta.1**, MIT, maintained by Engr. Raven Magbanua.
It includes both the C99 core and C++11 wrapper; no separate C package is needed.

Target repository: **pravennn08/SerialLedger**, public on GitHub.
Target PlatformIO package: **engrpraven08/SerialLedger**.
The board repository is public and tag `v0.1.0-beta.1` points to the commit with
green CI. PlatformIO accepted the upload and its registry listing is now live.
Arduino Library Manager registration was accepted and
[PR #9367](https://github.com/arduino/library-registry/pull/9367) was merged.
The library becomes installable after Arduino's index updates; refresh Library
Manager and search for **SerialLedger** by **Raven Magbanua**.

## Prepare the standalone repository

Arduino Library Manager requires `library.properties` at the repository root.
Do not submit the development repository, which keeps it under `library/`.
From the development repository root:

```powershell
uv run --locked python scripts/prepare_board_registry.py
```

The export is `work/board-registry/SerialLedger/` and contains:

```text
SerialLedger/
  library.properties
  library.json
  LICENSE.md
  README.md
  CMakeLists.txt
  UPSTREAM.md
  src/
  examples/BasicTelemetry/BasicTelemetry.ino
  docs/BOARD_API.md
  .github/workflows/ci.yml
```

The exporter refuses to overwrite an existing directory. For another export,
use `--output work/board-registry-next/SerialLedger`. It copies the source/example
files unchanged, adjusts the library URLs to the standalone repository, and
preserves the license and beta version. It creates no Git repository or tags
and performs no publication. Existing files under `dist/` are not changed.

Review and commit the development changes before making a final export so that
`UPSTREAM.md` identifies the source commit. Develop future API changes in the
development repository and export them here for review; do not maintain two
independent implementations.

## Create the public GitHub repository

Log in from a normal local PowerShell terminal if needed:

```powershell
gh auth login --hostname github.com --git-protocol https --web
gh auth status
```

Choose **Yes** if asked to authenticate Git, and complete the browser login as
`pravennn08`. Do not paste credentials into chat.
Create an empty public **SerialLedger** repository, or use these commands from
the freshly exported folder:

```powershell
git init -b main
git add .
git commit -m "Prepare SerialLedger C/C++ beta for library registries"
gh repo create pravennn08/SerialLedger --public --source . --remote origin --push
```

If you created the empty repository on the GitHub website instead, use
`git remote add origin https://github.com/pravennn08/SerialLedger.git` followed
by `git push -u origin main`, instead of `gh repo create`.
Avoid adding an initial README/license on GitHub: the export supplies them.

Wait for **Board library CI** to pass: Arduino submission lint, PlatformIO packing,
and example compilation for Uno, classic Nano, and ESP32. Then create the tag:

```powershell
git tag v0.1.0-beta.1
git push origin v0.1.0-beta.1
```

This tag belongs to the new board repository; it does not replace the development
repository's existing beta tag. A GitHub prerelease can be added but is not
required for Arduino submission. Check that the repository and tag are readable
while logged out. Never change the existing PyPI beta files to accompany this step.

## Submit to Arduino Library Manager

The prepared local registry checkout is `work/arduino-library-registry`, branch
`add-serialledger-library`. Its commit adds only the board repository URL to
`repositories.txt`. The PR description is `work/arduino-submission-body.md`.
From a normal PowerShell terminal with GitHub CLI logged in as `pravennn08`, run:

```powershell
& .\scripts\submit_arduino_library.ps1
```

The script checks the public beta tag, creates/reuses the official registry fork,
pushes the prepared branch, and opens the submission PR. It reuses an existing
submission instead of creating a duplicate. Follow the resulting bot checks.
This step needs the prepared local checkout/body; the manual procedure below
also works from another machine.

1. Fork <https://github.com/arduino/library-registry>.
2. Create a submission branch in that fork.
3. Add exactly this line to `repositories.txt`, leaving existing entries unchanged:

    ```text
    https://github.com/pravennn08/SerialLedger
    ```

4. Open a pull request to `arduino/library-registry:main` with title
   **Add SerialLedger library** and description:

    > Adds SerialLedger, an MIT-licensed C99/C++11 library for sending typed
    > newline-delimited serial telemetry. Repository contains root-level library
    > metadata and tag v0.1.0-beta.1. Tested on Arduino Uno, classic Nano, and ESP32.

5. Follow the bot checks and resolve any errors. After acceptance/indexing,
   search **SerialLedger** by **Raven Magbanua** in Arduino Library Manager.

The name must be unique in Arduino's index, regardless of case. The current
name check found no SerialLedger entry; recheck when submitting. Future releases
need a new `library.properties` version and Git tag; the indexer checks tags
automatically after the initial repository submission is accepted.

## Publish to PlatformIO

Version `0.1.0-beta.1` is already published. The commands below describe the
procedure for a future version; do not republish the current name/version.

Use the PlatformIO terminal in VS Code. If `pio` is not on PATH on Windows, the
installed Core can be invoked as:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m platformio --version
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m platformio account login
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m platformio account show
```

Run these commands from the standalone exported repository folder:

```sh
pio account login
pio account show
pio pkg search "owner:engrpraven08 name:SerialLedger"
pio pkg pack . --output ../SerialLedger-0.1.0-beta.1.tar.gz
pio pkg publish ../SerialLedger-0.1.0-beta.1.tar.gz --owner engrpraven08 --type library
```

Confirm that the logged-in account owns `engrpraven08`. Inspect the packed tarball
before publishing; its manifest must be at the package root with version
`0.1.0-beta.1`, MIT license, sources, and example. The manifest's export list
excludes CI, Git metadata, and build products. Publish the reviewed tarball.
Keep the interactive publication confirmation enabled. Do not publish from the
monorepo root. PlatformIO cannot reuse a published name/version even after removal.

After the registry processes it, use this in a fresh test project's environment:

```ini
lib_deps =
    engrpraven08/SerialLedger@0.1.0-beta.1
```

Remove the test project's old local/symlink SerialLedger dependency so the build
actually exercises the registry package. Build the included example for your
board and check sensor recording/export with the separately installed Python CLI.

## Sources

- [Arduino submission requirements](https://github.com/arduino/library-registry/blob/main/FAQ.md#submission-requirements)
- [Arduino submission procedure](https://github.com/arduino/library-registry#adding-a-library-to-library-manager)
- [PlatformIO publishing](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_publish.html)
- [PlatformIO packing](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_pack.html)
