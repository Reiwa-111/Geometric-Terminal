# GeoTerminal 2.2

**A native interactive geometry workbench for Windows.** GeoTerminal combines parametric 2D/3D models, geometric constructions, plane sections, vector measurements and an interactive unit-circle lab in a compact C++20 desktop application built with wxWidgets 3.2 and OpenGL.

> **Documentation:** [English](README.md) · [Русский](README.ru.md)

## Features

- **15 parametric models:** isosceles and right triangles, square, rectangle, circle, regular polygon, cube, cuboid, tetrahedron, octahedron, cylinder, cone, prism, pyramid and sphere.
- **Live model metrics:** vertex/edge/face counts and model-defined lengths, areas, perimeters, diagonals and volumes.
- **Interactive 3D viewport:** orbit, pan, zoom, isometric/top/front/right views, perspective and orthographic projection.
- **Three display modes:** shaded surfaces, wireframe with hidden edges, and shaded surfaces with an edge overlay.
- **Selection:** inspect vertices, edges and faces; view point coordinates and selected-edge length.
- **Constructions:** user points, segments, lines and rays. Points can be placed on XY or entered with explicit XYZ coordinates.
- **Plane sections:** axis-aligned or custom planes, intersection polygon, highlighted section overlay, vertex count, area and perimeter.
- **Vector analysis:** vector `AB`, its length, midpoint and angles to the coordinate axes.
- **Trigonometry:** interactive unit circle, negative and multi-turn angles, sine/cosine/tangent values and exact π notation for common angles.
- **Portable Windows build:** `portable/` contains the executable and bundled runtime DLLs.

## Quick start

1. Open [`portable/`](portable/).
2. Run `GeoTerminal.exe`.

The portable build does not require Python, MSYS2 or a separate wxWidgets installation. Keep the DLL files next to the executable.

Alternatively, run `./run.ps1` from PowerShell. If the portable executable is missing, the script invokes the build script.

## Build from source

The supplied build script targets Windows 10/11 and MSYS2 **UCRT64**. It expects CMake 3.24+, GCC and MinGW Make in `C:\msys64\ucrt64`, wxWidgets 3.2 (`core`, `base`, `gl`) and OpenGL development libraries.

Run from the repository root:

```powershell
./build.ps1
```

The script configures CMake, builds the application and geometry tests, runs CTest, then recreates `portable/` and copies the required runtime DLLs beside the executable.

For a manual build/test cycle after configuration:

```powershell
cmake --build build-native --parallel 6
ctest --test-dir build-native --output-on-failure
```

See [Building and development](docs/BUILDING.en.md) for setup and troubleshooting.

## Controls at a glance

| Action | Control |
|---|---|
| Orbit camera | Middle mouse button + drag, in any tool |
| Pan camera | Right mouse button + drag |
| Zoom | Mouse wheel |
| Orbit in navigation mode | Left mouse button + drag |
| Use active tool | Left click |
| Preset views | `0` isometric, `1` top XY, `2` front XZ, `3` right YZ |
| Toggle grid / axes / labels | `G` / `A` / `L` |
| Navigation / select / point | `O` / `S` / `P` |
| Segment / line / ray | `T` / `N` / `Y` |
| Reset camera | `R` |
| Delete selected user point | `Delete` |
| Return to navigation tool | `Esc` |

Hotkeys are passed through while a text or numeric input has focus. See the [user guide](docs/USER_GUIDE.en.md) for tool workflows and geometry details.

## Project layout

```text
src/       Geometry core, OpenGL viewport, trig panel and wxWidgets UI
tests/     Geometry and plane-intersection tests
tools/     Runtime DLL collection script
portable/  Ready-to-run Windows executable and runtime DLLs
docs/      English and Russian user, build and architecture guides
```

Geometry calculations are separated from the GUI. Rendering uses `wxGLCanvas` and OpenGL; the project does not embed a browser engine or a heavyweight 3D engine.

## Current scope and limitations

- The current interface does not provide project-file save/load. Geometry is generated interactively and is not persisted as a document.
- Segment, line and ray tools place their endpoints on the XY work plane. Use XYZ fields for points elsewhere in 3D space.
- A plane section is calculated from mesh edges and displayed as a highlighted polygon overlay. The current renderer does not perform a Boolean cut that removes one side of the solid.
- Curved objects are represented by polygonal meshes, so their sections are mesh approximations.
- The supplied build and portable package are Windows-oriented. The checked-in PowerShell packaging scripts target MSYS2 UCRT64.

## License

GeoTerminal is distributed under the [Apache License 2.0](LICENSE).

## More documentation

- [User guide (English)](docs/USER_GUIDE.en.md) · [Руководство пользователя (русский)](docs/USER_GUIDE.ru.md)
- [Build and development (English)](docs/BUILDING.en.md) · [Сборка и разработка (русский)](docs/BUILDING.ru.md)
- [Architecture (English)](docs/ARCHITECTURE.en.md) · [Архитектура (русский)](docs/ARCHITECTURE.ru.md)
