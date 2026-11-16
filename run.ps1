$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot 'portable\GeoTerminal.exe'
if (-not (Test-Path $exe)) { & (Join-Path $PSScriptRoot 'build.ps1') }
Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
