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

$Py = Get-Command py -ErrorAction SilentlyContinue
if ($Py) {
  $PythonExe = $Py.Source
  $PythonPrefix = @("-3")
} else {
  $Python = Get-Command python3 -ErrorAction SilentlyContinue
  if (-not $Python) { $Python = Get-Command python -ErrorAction SilentlyContinue }
  if (-not $Python) { Write-Error "Python 3 was not found on PATH" }
  $PythonExe = $Python.Source
  $PythonPrefix = @()
}

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

$fwArgs = $PythonPrefix + @($FwPy)
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
  & $PythonExe @fwArgs
  if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
  }
} finally {
  Pop-Location
}

Write-Host "Done."
