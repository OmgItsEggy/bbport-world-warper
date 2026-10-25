$ErrorActionPreference = 'Stop'
try {
    $warperRoot = $PSScriptRoot
    $warperSettings = Join-Path $warperRoot 'settings.json'
    $warperInstall = ''
    if (Test-Path -LiteralPath $warperSettings) {
        $warperInstall = (Get-Content -LiteralPath $warperSettings -Raw | ConvertFrom-Json).install
    }
    if (-not $warperInstall -or -not (Test-Path -LiteralPath (Join-Path $warperInstall 'Bloodborne.exe'))) {
        $warperInstall = (Read-Host 'Paste your bbport-windows folder (contains Bloodborne.exe)').Trim().Trim('"')
    }
    $warperPython = Join-Path $warperInstall 'Bloodborne.exe'
    if (-not (Test-Path -LiteralPath $warperPython)) { throw 'Bloodborne.exe was not found in that folder.' }
    & $warperPython --script (Join-Path $warperRoot 'tools\friend_setup.py') --install $warperInstall
    if ($LASTEXITCODE) { throw "World Warper exited with code $LASTEXITCODE. See the message above and build\fresh-character\smoke.log." }
} catch {
    Write-Host $_ -ForegroundColor Red
    exit 1
}
