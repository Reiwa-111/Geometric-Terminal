# Building and Development

[English](BUILDING.en.md) · [Русский](BUILDING.ru.md) · [Project README](../README.md)

## Supported build path

The repository currently provides a Windows build and packaging path based on MSYS2 UCRT64. The root `build.ps1` script assumes the toolchain is installed at `C:\msys64\ucrt64`.

### Prerequisites

- Windows 10 or Windows 11;
- CMake 3.24 or newer;
- MSYS2 UCRT64 with GCC, MinGW Make and `objdump`;
- wxWidgets 3.2 CMake package with `core`, `base` and `gl` components;
- OpenGL development libraries.

| Tool | Expected path |
|---|---|
| GCC | `C:\msys64\ucrt64\bin\g++.exe` |
| MinGW Make | `C:\msys64\ucrt64\bin\mingw32-make.exe` |
| MSYS2 prefix | `C:\msys64\ucrt64` |
| Build directory | `build-native/` |
| Portable output | `portable/` |

If MSYS2 is installed elsewhere, update `$ucrt` in `build.ps1` and pass the same prefix to `tools/runtime_deps.ps1`.

## Build and test

From PowerShell, in the repository root:

```powershell
./build.ps1
```

The script prepends the UCRT64 `bin` directory to `PATH`, configures CMake with the MinGW Makefiles generator and Release configuration, builds the GUI and geometry tests, runs CTest, then recreates `portable/`, copies `GeoTerminal.exe` and collects non-system runtime DLL dependencies using `objdump`.

The script deletes and recreates the existing `portable/` directory. Close a running `GeoTerminal.exe` before building, otherwise Windows may prevent the executable from being replaced.

For an incremental build and test run after CMake has configured the build directory:

```powershell
cmake --build build-native --parallel 6
ctest --test-dir build-native --output-on-failure
```

To run the application:

```powershell
./run.ps1
```

`run.ps1` starts `portable/GeoTerminal.exe`; if that file is absent, it invokes `build.ps1` first.

## Runtime packaging

`tools/runtime_deps.ps1` inspects PE import tables using UCRT64 `objdump`. It recursively copies required non-system DLLs from the UCRT64 `bin` directory into the destination. Windows system DLLs and API-set DLL names are not bundled.

The portable directory must keep the executable and its DLLs together:

```text
portable/
  GeoTerminal.exe
  *.dll
```

Do not copy arbitrary DLLs from another compiler environment into this package. The bundled runtime set is derived from the executable built by the configured toolchain.

## Tests

CTest currently registers one test executable, `GeoTerminalTests`, under the test name `geometry-core`. It exercises creation of all 15 supported model types, mesh topology and Euler checks for closed solids, outward face winding, selected model metrics, axis-aligned/diagonal/three-point plane intersections, section vertices/area/perimeter for a hexagonal pyramid, exact radian formatting and vector operations.

Run the tests with output on failure:

```powershell
ctest --test-dir build-native --output-on-failure
```

The current automated target is a geometry-core test. It does not constitute automated GUI interaction, visual regression or installer acceptance testing.

## Source layout

```text
src/
  geometry.hpp / geometry.cpp       Geometry primitives, meshes and calculations
  viewport.hpp / viewport.cpp       OpenGL viewport, camera, picking and constructions
  trig_circle.hpp / trig_circle.cpp Unit-circle drawing
  main_frame.hpp / main_frame.cpp   wxWidgets window, controls and application logic
  main.cpp                          Application entry point
tests/
  geometry_tests.cpp                Geometry-core regression tests
tools/
  runtime_deps.ps1                  Portable runtime dependency collector
```

## Development notes

- Keep geometry calculations independent of wxWidgets and OpenGL where practical. `geometry.cpp` is built into both the GUI and the standalone test executable.
- Keep portable output reproducible through `build.ps1` and `runtime_deps.ps1`; avoid manually adding unrelated runtime DLLs.
- `.gitignore` excludes build directories and logs. `portable/` is intentionally versioned because the repository distributes the ready-to-run Windows build.
- Update both language versions of the README and docs when changing user-visible behavior or build requirements.
- Do not describe a visual plane-section overlay as a Boolean mesh cut. The current implementation computes an intersection polygon from mesh edges and draws it as a foreground highlight.

## Troubleshooting

**CMake cannot find wxWidgets:** confirm that the UCRT64 wxWidgets 3.2 CMake package is installed and `CMAKE_PREFIX_PATH` points to `C:\msys64\ucrt64`. The project requests `core`, `base` and `gl`.

**`objdump` or a runtime DLL is missing:** confirm that `C:\msys64\ucrt64\bin\objdump.exe` exists and the UCRT64 runtime packages are installed. Packaging intentionally fails when a non-system dependency cannot be resolved.

**The portable executable cannot be replaced:** close all GeoTerminal processes and run `./build.ps1` again. The script recreates `portable/` after tests pass.

**CTest fails:** run the test executable directly from `build-native/` or rerun CTest with `--output-on-failure`. The failing assertion name identifies the geometry invariant to investigate.
