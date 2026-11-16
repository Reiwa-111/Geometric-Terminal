param(
    [Parameter(Mandatory=$true)][string]$Binary,
    [Parameter(Mandatory=$true)][string]$Destination,
    [string]$Ucrt = 'C:\msys64\ucrt64'
)
$ErrorActionPreference = 'Stop'
$binDir = Join-Path $Ucrt 'bin'
$objdump = Join-Path $binDir 'objdump.exe'
if (-not (Test-Path $objdump)) { throw "objdump not found: $objdump" }
New-Item -ItemType Directory -Force -Path $Destination | Out-Null

$systemDlls = @(
    'kernel32.dll','user32.dll','gdi32.dll','opengl32.dll','shell32.dll','ole32.dll','oleaut32.dll',
    'comdlg32.dll','comctl32.dll','uuid.dll','rpcrt4.dll','ws2_32.dll','winmm.dll','version.dll',
    'imm32.dll','uxtheme.dll','shcore.dll','dwmapi.dll','crypt32.dll','bcrypt.dll','winspool.drv',
    'setupapi.dll','cfgmgr32.dll','ntdll.dll','msimg32.dll','advapi32.dll','gdi32full.dll','msvcrt.dll',
    'sechost.dll','combase.dll','oleacc.dll','dwrite.dll','dxgi.dll','d3d9.dll','windowscodecs.dll',
    'propsys.dll','shlwapi.dll'
)
$visited = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$queue = [System.Collections.Generic.Queue[string]]::new()
$queue.Enqueue((Resolve-Path $Binary).Path)
$copied = [System.Collections.Generic.List[string]]::new()

while ($queue.Count -gt 0) {
    $current = $queue.Dequeue()
    $lines = & $objdump -p $current
    if ($LASTEXITCODE -ne 0) { throw "objdump failed: $current" }
    foreach ($line in $lines) {
        if ($line -notmatch 'DLL Name:\s*(\S+)') { continue }
        $name = $Matches[1]
        if ($visited.Contains($name)) { continue }
        [void]$visited.Add($name)
        if ($systemDlls -contains $name.ToLowerInvariant() -or $name -match '^(api|ext)-ms-win-') { continue }
        $source = Join-Path $binDir $name
        if (-not (Test-Path $source)) { throw "Unresolved runtime DLL '$name' required by '$current'" }
        $target = Join-Path $Destination $name
        if (-not (Test-Path $target)) {
            Copy-Item -LiteralPath $source -Destination $target
            $copied.Add($name)
        }
        $queue.Enqueue($source)
    }
}

Write-Host "Runtime DLLs bundled: $($copied.Count)"
$copied | Sort-Object | ForEach-Object { Write-Host "  $_" }
