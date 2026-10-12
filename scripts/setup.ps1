[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path $PSScriptRoot -Parent
$taskOldTemp = $env:TEMP
$taskOldTmp = $env:TMP
$taskOldUvCache = $env:UV_CACHE_DIR
$taskOldUvLink = $env:UV_LINK_MODE
Push-Location $taskProjectRoot
try {
    if (-not (Get-Command uv -ErrorAction SilentlyContinue)) {
        throw 'Install uv first: https://docs.astral.sh/uv/getting-started/installation/'
    }
    New-Item -ItemType Directory -Path 'work/tmp' -Force | Out-Null
    $env:TEMP = Join-Path $taskProjectRoot 'work/tmp'
    $env:TMP = $env:TEMP
    $env:UV_CACHE_DIR = Join-Path $taskProjectRoot 'work/uv-cache'
    $env:UV_LINK_MODE = 'copy'
    uv sync --locked --no-python-downloads
    if ($LASTEXITCODE -ne 0) { throw 'uv sync failed' }
    $taskCmakeBin = '.venv/Lib/site-packages/cmake/data/bin'
    & "$taskCmakeBin/cmake.exe" --preset windows-msvc
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
    & "$taskCmakeBin/cmake.exe" --build --preset windows-debug
    if ($LASTEXITCODE -ne 0) { throw 'C/C++ build failed' }
    & "$taskCmakeBin/ctest.exe" --preset windows-debug
    if ($LASTEXITCODE -ne 0) { throw 'Native tests failed' }
    uv run --locked python -m unittest discover -s tests/python -v
    if ($LASTEXITCODE -ne 0) { throw 'Python tests failed' }
    uv run --locked ruff check desktop tests/python
    if ($LASTEXITCODE -ne 0) { throw 'Python lint failed' }
    Write-Host 'SerialLedger setup complete. Run: uv run python -m serial_ledger demo'
} finally {
    $env:TEMP = $taskOldTemp
    $env:TMP = $taskOldTmp
    $env:UV_CACHE_DIR = $taskOldUvCache
    $env:UV_LINK_MODE = $taskOldUvLink
    Pop-Location
}
