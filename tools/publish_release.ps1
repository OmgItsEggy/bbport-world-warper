param(
    [Parameter(Mandatory=$true)][string]$AssetsRoot,
    [string]$Tag = 'v0.1.0-alpha.1',
    [string]$Commit = 'main',
    [switch]$CheckOnly,
    [switch]$EnablePages
)
$ErrorActionPreference = 'Stop'
$env:GCM_INTERACTIVE = 'never'
$env:GIT_TERMINAL_PROMPT = '0'
# Use the existing GitHub credential only in memory. Never print/store the token.
$warperCredentialLines = @('protocol=https', 'host=github.com', '') | git credential fill
if ($LASTEXITCODE) { throw 'GitHub credentials are unavailable.' }
$warperToken = ($warperCredentialLines | Where-Object { $_.StartsWith('password=') }) -replace '^password=', ''
if (-not $warperToken) { throw 'No GitHub token was returned.' }
$warperHeaders = @{ Authorization = "Bearer $warperToken"; Accept = 'application/vnd.github+json'; 'X-GitHub-Api-Version' = '2022-11-28' }
$warperApi = 'https://api.github.com/repos/OmgItsEggy/bbport-world-warper'
$warperRepo = Invoke-RestMethod -Uri $warperApi -Headers $warperHeaders
if (-not $warperRepo.permissions.push) { throw 'This account cannot publish to the project.' }
if ($EnablePages) {
    try {
        $warperPages = Invoke-RestMethod -Uri "$warperApi/pages" -Headers $warperHeaders
        if ($warperPages.build_type -ne 'workflow') {
            Invoke-RestMethod -Method Put -Uri "$warperApi/pages" -Headers $warperHeaders -ContentType 'application/json' -Body '{"build_type":"workflow"}' | Out-Null
        }
    } catch {
        if ([int]$_.Exception.Response.StatusCode -ne 404) { throw }
        Invoke-RestMethod -Method Post -Uri "$warperApi/pages" -Headers $warperHeaders -ContentType 'application/json' -Body '{"build_type":"workflow"}' | Out-Null
    }
    Write-Host 'GitHub Pages configured for the project deployment workflow.'
}
if ($CheckOnly) {
    [pscustomobject]@{ Repository=$warperRepo.full_name; CanPublish=$warperRepo.permissions.push; DefaultBranch=$warperRepo.default_branch } | ConvertTo-Json
    exit 0
}
$warperResolvedAssets = (Resolve-Path -LiteralPath $AssetsRoot).Path
$warperAssets = @(
    Join-Path $warperResolvedAssets "WorldWarper-$Tag-windows-x64.zip"
    Join-Path $warperResolvedAssets "WorldWarper-$Tag-corresponding-source.zip"
    Join-Path $warperResolvedAssets 'SHA256SUMS.txt'
)
foreach ($warperAsset in $warperAssets) {
    if (-not (Test-Path -LiteralPath $warperAsset -PathType Leaf)) { throw "Missing release asset: $warperAsset" }
}
$warperNotes = @'
Experimental Windows x64 add-on for prepared Supermedo bbport v1.9.4 + Bloodborne 1.09.

Download windows-x64.zip, extract separately, and double-click Start-WorldWarper.cmd.
Full warm-up runs weapons/armour + all 44 lantern destinations + the experimental boss/phase batch automatically.

Read QUICKSTART.md first. Setup remembers your folders and uses Python/runtime DLLs already in bbport; no compiler or separate Python installation is needed. It imports the original Nexus mod 619/file 3156 ZIP locally. The save author forbids redistribution, so the save itself is not bundled. Never install it over your personal save.

An isolated test copy is reset before every game process, including retries. Normal GPU caches are reused. Live saves and installed game binaries are not replaced. Online is disabled for the tour.

This is NOT complete shader/attack/boss coverage. Known failures are retained in reports. Micolash is excluded; Gehrman/Moon Presence entry is not implemented; Chalice bosses are out of scope. See docs/STATUS.md for current evidence and remaining limitations.

The corresponding-source ZIP contains the complete modified port, pinned submodule files, World Warper sources/tests and build instructions. Runtime DLLs and game files are not redistributed. SHA256SUMS.txt verifies both archives.
'@
$warperBody = @{ tag_name=$Tag; target_commitish=$Commit; name="World Warper $Tag (experimental)"; body=$warperNotes; draft=$true; prerelease=$true } | ConvertTo-Json
$warperRelease = Invoke-RestMethod -Method Post -Uri "$warperApi/releases" -Headers $warperHeaders -ContentType 'application/json' -Body $warperBody
foreach ($warperAsset in $warperAssets) {
    $warperAssetName = [IO.Path]::GetFileName($warperAsset)
    $warperUploadUri = ($warperRelease.upload_url -replace '\{.*\}$', '') + '?name=' + [Uri]::EscapeDataString($warperAssetName)
    $warperUploaded = Invoke-RestMethod -Method Post -Uri $warperUploadUri -Headers $warperHeaders -ContentType 'application/octet-stream' -InFile $warperAsset -TimeoutSec 600
    if ($warperUploaded.size -ne (Get-Item -LiteralPath $warperAsset).Length) { throw "Upload size mismatch: $warperAssetName; release left as draft." }
    Write-Host "Uploaded $warperAssetName"
}
$warperPublished = Invoke-RestMethod -Method Patch -Uri "$warperApi/releases/$($warperRelease.id)" -Headers $warperHeaders -ContentType 'application/json' -Body '{"draft":false,"prerelease":true}'
Write-Host "Published $($warperPublished.html_url)"
