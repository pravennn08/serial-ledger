# Run in a normal PowerShell terminal so gh can access the Windows login keyring.
# Submits the prepared one-line registry change; it does not modify library releases.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$submissionRoot = Split-Path $PSScriptRoot -Parent
$submissionCheckout = Join-Path $submissionRoot 'work/arduino-library-registry'
$submissionBody = Join-Path $submissionRoot 'work/arduino-submission-body.md'
$submissionOwner = 'pravennn08'
$submissionBranch = 'add-serialledger-library'
$submissionForkUrl = 'https://github.com/pravennn08/library-registry.git'

$submissionLogin = gh api user --jq .login
if ($LASTEXITCODE -ne 0 -or $submissionLogin.Trim() -ne $submissionOwner) {
    throw 'Log in to GitHub CLI as pravennn08 in this terminal before submitting.'
}
if (-not (Test-Path -LiteralPath $submissionBody)) { throw 'Prepared PR body is missing.' }
$submissionCurrentBranch = git -C $submissionCheckout branch --show-current
if ($LASTEXITCODE -ne 0 -or $submissionCurrentBranch.Trim() -ne $submissionBranch) {
    throw 'Prepared Arduino submission branch is missing.'
}
$submissionChanges = git -C $submissionCheckout status --porcelain
if ($LASTEXITCODE -ne 0 -or $submissionChanges) { throw 'Submission checkout has uncommitted changes.' }
$submissionTag = Invoke-RestMethod -Uri 'https://api.github.com/repos/pravennn08/SerialLedger/git/ref/tags/v0.1.0-beta.1'
if ($submissionTag.object.sha -ne 'b4b74a5b717d244de4befbf604e76093c7841703') {
    throw 'The public beta tag differs from the CI-verified commit.'
}

$submissionForkInfo = $null
try {
    $submissionForkInfo = Invoke-RestMethod -Uri 'https://api.github.com/repos/pravennn08/library-registry'
} catch {
    if ([int]$_.Exception.Response.StatusCode -ne 404) { throw }
}
if (-not $submissionForkInfo) {
    gh repo fork arduino/library-registry --clone=false
    if ($LASTEXITCODE -ne 0) { throw 'Could not create the Arduino registry fork.' }
} elseif ($submissionForkInfo.parent.full_name -ne 'arduino/library-registry') {
    throw 'pravennn08/library-registry exists but is not an Arduino registry fork.'
}
$submissionRemotes = git -C $submissionCheckout remote
if ($submissionRemotes -notcontains 'fork') {
    git -C $submissionCheckout remote add fork $submissionForkUrl
    if ($LASTEXITCODE -ne 0) { throw 'Could not add the fork remote.' }
} else {
    $submissionRemoteUrl = git -C $submissionCheckout remote get-url fork
    if ($submissionRemoteUrl.Trim() -ne $submissionForkUrl) { throw 'Unexpected fork remote URL.' }
}
git -C $submissionCheckout push fork $submissionBranch
if ($LASTEXITCODE -ne 0) { throw 'Could not push the prepared submission branch.' }

$submissionExisting = gh pr list --repo arduino/library-registry --head $submissionBranch --author $submissionOwner --state all --json url
if ($LASTEXITCODE -ne 0) { throw 'Could not check existing submissions.' }
$submissionExisting = $submissionExisting | ConvertFrom-Json
if ($submissionExisting.Count -gt 0) {
    Write-Output $submissionExisting[0].url
    return
}
gh pr create --repo arduino/library-registry --head "$submissionOwner`:$submissionBranch" --base main --title 'Add SerialLedger library' --body-file $submissionBody
if ($LASTEXITCODE -ne 0) { throw 'Could not create the Arduino submission pull request.' }
