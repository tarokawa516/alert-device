param(
    [ValidateSet('all', 'hirofumi', 'backroom')][string]$Target = 'all',
    [string]$ArduinoCli = 'arduino-cli',
    [switch]$ExampleSecrets,
    [switch]$EnableDisplays
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$targets = if ($Target -eq 'all') { @('hirofumi', 'backroom') } else { @($Target) }
if ($ArduinoCli -eq 'arduino-cli' -and -not (Get-Command $ArduinoCli -ErrorAction SilentlyContinue)) {
    $localCli = Join-Path $repoRoot '.tools\arduino-cli\arduino-cli.exe'
    if (Test-Path -LiteralPath $localCli) { $ArduinoCli = $localCli }
}
if (Test-Path -LiteralPath (Join-Path $repoRoot '.tools\arduino-user')) {
    $env:ARDUINO_DIRECTORIES_USER = Join-Path $repoRoot '.tools\arduino-user'
}
$env:ARDUINO_UPDATER_ENABLE_NOTIFICATION = 'false'
$fqbn = 'esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=default'
foreach ($sketchName in $targets) {
    # Stage only source files; never print credentials or reuse a stale secret.
    $stage = Join-Path $repoRoot ".build\staged\$sketchName"
    New-Item -ItemType Directory -Force -Path $stage | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $repoRoot $sketchName) -File |
        Where-Object { $_.Extension -in @('.ino', '.h', '.cpp') -and $_.Name -ne 'secret.h' } |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $stage -Force }
    $secretSource = if ($ExampleSecrets) { Join-Path $repoRoot 'secret_example.h' } else { Join-Path $repoRoot "$sketchName\secret.h" }
    if (-not (Test-Path -LiteralPath $secretSource)) { throw "Missing $sketchName/secret.h. Copy secret_example.h or build with -ExampleSecrets." }
    Copy-Item -LiteralPath $secretSource -Destination (Join-Path $stage 'secret.h') -Force
    if ($EnableDisplays) {
        $configPath = Join-Path $stage 'config.h'
        $configText = (Get-Content -Raw -LiteralPath $configPath) -replace 'ENABLE_DISPLAY = false', 'ENABLE_DISPLAY = true' -replace 'ENABLE_TOUCH = false', 'ENABLE_TOUCH = true'
        [IO.File]::WriteAllText($configPath, $configText, [Text.UTF8Encoding]::new($false))
    }
    # Copy-Item may preserve old timestamps. Force dependency invalidation,
    # especially when switching from local credentials to example credentials.
    Get-ChildItem -LiteralPath $stage -File | ForEach-Object { $_.LastWriteTimeUtc = [DateTime]::UtcNow }
    $profile = if ($EnableDisplays) { 'displays' } else { 'default' }
    # Reuse compilation objects when checking the display-enabled profile.
    $buildDir = Join-Path $repoRoot ".build\default\$sketchName"
    $outputDir = Join-Path $repoRoot ".build\firmware\$profile\$sketchName"
    & $ArduinoCli compile --fqbn $fqbn --library (Join-Path $repoRoot 'common\AlertDevice') --warnings all --jobs 4 --build-path $buildDir --output-dir $outputDir $stage
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $sketchName" }
}
