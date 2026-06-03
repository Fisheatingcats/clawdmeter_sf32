param(
    [string]$ComPort = "",
    [switch]$SkipDownload
)

$ErrorActionPreference = "Stop"

$ProjectDir = $PSScriptRoot
$SdkRoot = Resolve-Path (Join-Path $ProjectDir "..\..\..\sifli-sdk")
$BuildDir = Join-Path $ProjectDir "build_sf32lb52-lchspi-ulp_hcpu"
$Board = "sf32lb52-lchspi-ulp"

Push-Location $SdkRoot
. .\export.ps1
Pop-Location

Push-Location $ProjectDir
scons --board=$Board
if ($LASTEXITCODE -ne 0) {
    Pop-Location
    throw "scons failed with exit code $LASTEXITCODE"
}
Pop-Location

if ($SkipDownload) {
    Write-Host "Build completed. UART download skipped."
    exit 0
}

if ([string]::IsNullOrWhiteSpace($ComPort)) {
    $ComPort = Read-Host "Input serial port, e.g. COM9 or 9. Empty to skip"
}

if ([string]::IsNullOrWhiteSpace($ComPort)) {
    Write-Host "Build completed. UART download skipped."
    exit 0
}

if ($ComPort -match '^\d+$') {
    $ComPort = "COM$ComPort"
}

Push-Location $BuildDir
& sftool -p $ComPort -c SF32LB52 -m nor write_flash `
    "bootloader\bootloader.bin@0x12010000" `
    "main.bin@0x12020000" `
    "ftab\ftab.bin@0x12000000"
if ($LASTEXITCODE -ne 0) {
    Pop-Location
    throw "sftool failed with exit code $LASTEXITCODE"
}
Pop-Location
