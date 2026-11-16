$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$ucrt = 'C:\msys64\ucrt64'
$env:Path = "$ucrt\bin;$env:Path"
$build = Join-Path $root 'build-native'
$portable = Join-Path $root 'portable'

& cmake -S $root -B $build -G 'MinGW Makefiles' `
    "-DCMAKE_CXX_COMPILER=$ucrt/bin/g++.exe" `
    "-DCMAKE_MAKE_PROGRAM=$ucrt/bin/mingw32-make.exe" `
    "-DCMAKE_PREFIX_PATH=$ucrt" `
    '-DCMAKE_BUILD_TYPE=Release' '-DBUILD_TESTING=ON'
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }

& cmake --build $build --parallel 6
if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }

& ctest --test-dir $build --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Geometry tests failed: $LASTEXITCODE" }

if (Test-Path $portable) { Remove-Item -LiteralPath $portable -Recurse -Force }
New-Item -ItemType Directory -Force -Path $portable | Out-Null
Copy-Item (Join-Path $build 'GeoTerminal.exe') (Join-Path $portable 'GeoTerminal.exe') -Force
& (Join-Path $root 'tools\runtime_deps.ps1') -Binary (Join-Path $portable 'GeoTerminal.exe') -Destination $portable -Ucrt $ucrt

Write-Host "Portable app ready: $portable\GeoTerminal.exe"
