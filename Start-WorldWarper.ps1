param([switch]$SetupOnly)
$ErrorActionPreference = 'Stop'
try {
    $warperRoot = $PSScriptRoot
    $warperSettings = Join-Path $warperRoot 'settings.json'
    $warperInstall = ''
    $warperGame = ''
    if (Test-Path -LiteralPath $warperSettings) {
        $warperSaved = Get-Content -LiteralPath $warperSettings -Raw | ConvertFrom-Json
        $warperInstall = $warperSaved.install
        $warperGame = $warperSaved.game
    }
    if (-not $warperInstall -or -not (Test-Path -LiteralPath (Join-Path $warperInstall 'Bloodborne.exe'))) {
        $warperInstall = (Read-Host 'Paste your bbport-windows folder (contains Bloodborne.exe)').Trim().Trim('"')
    }
    $warperPython = Join-Path $warperInstall 'Bloodborne.exe'
    if (-not (Test-Path -LiteralPath $warperPython)) { throw 'Bloodborne.exe was not found in that folder.' }
    if (-not $warperGame -or -not (Test-Path -LiteralPath (Join-Path $warperGame 'eboot.bin'))) {
        $warperGame = (Read-Host 'Paste your game-data folder (contains eboot.bin and dvdroot_ps4)').Trim().Trim('"')
    }
    # The frozen GUI launcher has no reliable stdin. Keep every prompt in the
    # parent console and pass explicit arguments to its embedded Python.
    $warperArguments = @('--script', (Join-Path $warperRoot 'tools\friend_setup.py'), '--install', $warperInstall, '--game', $warperGame, '--no-prompt')
    if (-not (Test-Path -LiteralPath (Join-Path $warperRoot 'build\save-seed\savedata\1\CUSA00207\SPRJ0005\userdata0000'))) {
        Write-Host 'Download the original test save ZIP: https://www.nexusmods.com/bloodborne/mods/619?tab=files&file_id=3156'
        $warperSaveZip = (Read-Host 'Paste the downloaded ZIP path (do not install it over your save)').Trim().Trim('"')
        $warperArguments += @('--save-zip', $warperSaveZip)
    }
    if ($SetupOnly) {
        $warperArguments += '--check'
    } else {
        Write-Host '1: FULL warm-up (weapons/armour + 44 lanterns + bosses/phases)'
        Write-Host '2: Four-boss entrance diagnostic; 3: Bosses only; 4: Equipment/lanterns only'
        $warperChoice = Read-Host 'Choose 1, 2, 3 or 4 [1]'
        if (-not $warperChoice) { $warperChoice = '1' }
        $warperModes = @{'1'='full'; '2'='entry-check'; '3'='bosses'; '4'='all-lamps'}
        if (-not $warperModes.ContainsKey($warperChoice)) { throw 'Choose 1, 2, 3 or 4.' }
        $warperArguments += @('--mode', $warperModes[$warperChoice])
    }
    # A pipe also supplies valid stdout handles to PyInstaller's windowed EXE.
    & $warperPython @warperArguments 2>&1 | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE) { throw "World Warper exited with code $LASTEXITCODE. See the message above and build\fresh-character\smoke.log." }
} catch {
    Write-Host $_ -ForegroundColor Red
    exit 1
}
