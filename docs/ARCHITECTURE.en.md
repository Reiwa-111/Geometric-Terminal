# Architecture Notes

[English](ARCHITECTURE.en.md) · [Русский](ARCHITECTURE.ru.md) · [Project README](../README.md)

## Overview

GeoTerminal is a native C++20 desktop application. Its geometry calculations are kept in a small core module, while wxWidgets owns the desktop interface and `wxGLCanvas` provides the OpenGL viewport.

```text
main.cpp
  └── MainFrame (wxWidgets)
        ├── GeometryViewport (wxGLCanvas + OpenGL)
        │     ├── camera and projection
        │     ├── mesh rendering and depth handling
        │     ├── picking and construction interaction
        │     └── plane/section and vector overlays
        ├── Shape and construction controls
        └── TrigCirclePanel

geometry.cpp / geometry.hpp
  ├── Vec3 operations
  ├── parametric mesh generation
  ├── topology and model metrics
  ├── plane/mesh-edge intersection
  └── exact-radian formatting
```

## Modules

### Geometry core — `src/geometry.hpp`, `src/geometry.cpp`

The `geo` namespace contains a minimal `Vec3` type and operations (`dot`, `cross`, `length`, `normalized`), a mesh representation and model-generation functions. `Mesh` stores vertices, indexed edges, polygon faces, calculated metrics and a smooth-shading flag.

`makeMesh(kind, params)` creates the selected model. Face winding is normalized outward for closed solids, and unique edges are collected from face boundaries. `intersectPlane(mesh, normal, offset)` finds intersections between the plane and mesh edges, removes duplicate hits and orders the resulting polygon around its center.

This module does not depend on wxWidgets or OpenGL. The same implementation is compiled into the GUI and the geometry test executable.

### Viewport — `src/viewport.hpp`, `src/viewport.cpp`

`GeometryViewport` derives from `wxGLCanvas`. It owns the OpenGL context, camera state, display settings, selected mesh elements, user points, construction lines, cutting plane and optional vector.

The camera uses one orthonormal basis for rendering, screen projection, point placement and picking. This keeps interaction aligned with the rendered view. The viewport supports perspective and orthographic projections, preset orientations, orbit, pan and wheel zoom.

Rendering uses the fixed-function OpenGL pipeline. The depth buffer is used for shaded geometry, visible/hidden edge treatment and ordinary 3D labels. The section polygon is intentionally drawn as a foreground highlight; it does not modify the source mesh.

### Main window — `src/main_frame.hpp`, `src/main_frame.cpp`

`MainFrame` constructs the menus, toolbar, split layout, tabbed controls and settings dialog. It translates UI input into calls to `GeometryViewport` and the geometry core, and formats model metrics, selection information, section statistics, vector data and trigonometric readouts.

### Trigonometry panel — `src/trig_circle.hpp`, `src/trig_circle.cpp`

`TrigCirclePanel` draws the unit circle and current angle using wxWidgets painting. The main frame supplies the angle; the panel is separate from the OpenGL geometry viewport.

## Data flow

1. A shape selection or dimension change creates a `geo::Params` value in `MainFrame`.
2. `geo::makeMesh` returns a mesh and its model metrics.
3. `MainFrame` sends the mesh to `GeometryViewport` and updates the metrics panel.
4. Viewport input updates camera state, selection, user points or construction lines.
5. Plane controls call `geo::intersectPlane` for section statistics; the viewport uses the same calculation to draw the highlighted polygon.
6. Vector and angle controls update their respective viewport or trigonometry panel state.

## Tests

`tests/geometry_tests.cpp` is a standalone test program linked with `src/geometry.cpp`. CMake registers it with CTest as `geometry-core`. The tests cover model creation, mesh topology, outward winding, selected model metrics, plane intersections, exact radian conversion and vector operations.

The current test suite does not instantiate the GUI or OpenGL context. Visual behavior and input interaction therefore need separate manual or future automated UI checks.

## Design boundaries

- Geometry generation and intersection algorithms belong in the `geo` core, not in event handlers or rendering code.
- wxWidgets controls and dialogs belong in `MainFrame`.
- Camera transforms, viewport rendering and viewport-specific interaction belong in `GeometryViewport`.
- Unit-circle drawing belongs in `TrigCirclePanel`.
- A rendered section overlay must not be described as a clipped or Boolean-modified mesh unless the geometry data itself is actually changed.
