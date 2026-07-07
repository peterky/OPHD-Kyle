param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [string]$RepoRoot = ""
)

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
    $RepoRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path
}

$ErrorActionPreference = "Stop"

$buildDir = Join-Path $RepoRoot ".build\Release_x64_appOPHD"
$exePath = Join-Path $buildDir "appOPHD.exe"
$dataSource = Join-Path $RepoRoot "data"
$distRoot = Join-Path $RepoRoot "dist"
$packageName = "OutpostHD-KylesColony-v0.8.10-$Version-win64"
$packageDir = Join-Path $distRoot $packageName
$zipPath = Join-Path $distRoot "$packageName.zip"

if (-not (Test-Path $exePath)) {
    throw "Build output not found: $exePath`nBuild Release|x64 first."
}

if (-not (Test-Path $dataSource)) {
    throw "Data folder not found: $dataSource"
}

if (Test-Path $packageDir) {
    Remove-Item $packageDir -Recurse -Force
}

New-Item -ItemType Directory -Path $packageDir | Out-Null

Copy-Item $exePath $packageDir
Copy-Item (Join-Path $buildDir "*.dll") $packageDir
Copy-Item $dataSource (Join-Path $packageDir "data") -Recurse
Copy-Item (Join-Path $RepoRoot "LICENSE.md") $packageDir
Copy-Item (Join-Path $RepoRoot "FORK_CHANGELOG.md") $packageDir

$readmeDist = Join-Path $packageDir "README.txt"
$readmeSource = Join-Path $RepoRoot "README.md"
if (Test-Path $readmeSource) {
    Copy-Item $readmeSource (Join-Path $packageDir "README.md")
}

@(
"# Outpost HD: Kyle's Colony",
"",
"Build: v0.8.10-$Version",
"",
"1. Extract this folder anywhere",
"2. Run appOPHD.exe (keep data/ next to the exe)",
"3. Confirm the main menu shows v0.8.10-$Version",
"",
"Saves: %AppData%\LairWorks\OutpostHD\savegames\",
"",
"Requirements:",
"- Windows 10/11 x64",
"- Microsoft VC++ Redistributable x64 if the game fails to start",
"",
"See FORK_CHANGELOG.md for version notes.",
"See README.md for full fork documentation."
) | Set-Content -Path $readmeDist -Encoding UTF8

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path $packageDir -DestinationPath $zipPath -CompressionLevel Optimal

$fileCount = (Get-ChildItem $packageDir -File).Count
$dllCount = (Get-ChildItem $packageDir -Filter "*.dll").Count
Write-Host "Packaged $packageName"
Write-Host "  Directory: $packageDir"
Write-Host "  Zip:       $zipPath"
Write-Host "  Files:     $fileCount ($dllCount DLLs)"