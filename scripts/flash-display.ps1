# Build or flash the FreeWili display with the vendored wilibsp BSP.
#
# Usage:
#   .\scripts\flash-display.ps1
#   .\scripts\flash-display.ps1 -Iface 1
#   .\scripts\flash-display.ps1 -SyncOnly    # copy kit firmware → wilibsp only
#   .\scripts\flash-display.ps1 -BuildOnly

param(
  [int]$Iface = -1,
  [switch]$SyncOnly,
  [switch]$BuildOnly
)

$ErrorActionPreference = "Stop"

$KitRoot = Split-Path -Parent $PSScriptRoot
$Wilibsp = Join-Path $KitRoot "wilibsp"
$FwPy = Join-Path $Wilibsp "tools\fw.py"
$KitFirmware = Join-Path $KitRoot "firmware\openmicro"
$WilibspApp = Join-Path $Wilibsp "apps\openmicro"

if (-not (Test-Path $FwPy)) {
  Write-Error "wilibsp fw.py not found at $FwPy"
}

function Sync-Firmware {
  if (-not (Test-Path $KitFirmware)) {
    Write-Error "Kit firmware missing: $KitFirmware"
  }
  Write-Host "Syncing kit firmware → $WilibspApp"
  New-Item -ItemType Directory -Force -Path $WilibspApp | Out-Null
  robocopy $KitFirmware $WilibspApp /E /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
  # robocopy: 0-7 are success / partial success
  if ($LASTEXITCODE -ge 8) {
    Write-Error "robocopy failed with exit $LASTEXITCODE"
  }
  Write-Host "Sync complete."
}

# firmware/openmicro is the source of truth. Always synchronize it before a
# build or flash so the compiled application cannot silently drift from it.
Sync-Firmware
if ($SyncOnly) { exit 0 }

$fwArgs = @("-3", $FwPy)
if ($BuildOnly) {
  $fwArgs += @("build", "openmicro")
  Write-Host "Building openmicro via wilibsp…"
} else {
  $fwArgs += @("flash", "openmicro")
  if ($Iface -ge 0) {
    $fwArgs += @("--iface", "$Iface")
  }
  Write-Host "Flashing openmicro to display CPU via wilibsp…"
}

Push-Location $Wilibsp
try {
  & py @fwArgs
  if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
  }
} finally {
  Pop-Location
}

Write-Host "Done."
