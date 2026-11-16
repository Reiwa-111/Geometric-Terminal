# GeoTerminal 2.2 — User Guide

[English](USER_GUIDE.en.md) · [Русский](USER_GUIDE.ru.md) · [Project README](../README.md)

## 1. Interface overview

The main window contains a 3D viewport on the left and a tabbed control panel on the right:

- **Shape** — choose a model, edit its dimensions and inspect calculated metrics and topology counts.
- **Constructions** — create points and lines, define a cutting plane, inspect a section and configure a vector.
- **Trigonometry** — explore the unit circle and angle functions.
- **Help** — an in-app reference for navigation and shortcuts.

The top toolbar contains navigation and construction tools, preset camera views, a draw-mode selector and a projection selector.

## 2. Choosing and editing a model

GeoTerminal provides 15 models:

| 2D models | 3D models |
|---|---|
| Isosceles triangle | Cube |
| Right triangle | Cuboid |
| Square | Tetrahedron |
| Rectangle | Octahedron |
| Circle | Cylinder |
| Regular polygon | Cone |
|  | Prism |
|  | Pyramid |
|  | Sphere |

Select a model in the **Shape** tab. Its dimension fields update the model immediately. The available parameters depend on the selected shape:

- `a` is the side length for regular polygons, prisms and pyramids.
- `a` is the diameter for a circle, cylinder, cone and sphere.
- `b`, `c` and `h` provide additional dimensions required by other models.
- `n` is the number of base sides for regular polygons, prisms and pyramids; the control allows 3–64 sides.

The metrics panel lists values defined by the selected model. The topology summary reports `V` (vertices), `E` (edges) and `F` (faces). A circle and sphere are represented by polygonal meshes for rendering and intersection calculations.

## 3. Navigating the viewport

| Action | Input |
|---|---|
| Orbit camera | Middle mouse button + drag, in any tool |
| Pan camera | Right mouse button + drag |
| Zoom | Mouse wheel |
| Orbit while NAV is active | Left mouse button + drag |
| Run the active tool | Left click |

Preset views are available from the toolbar and keyboard: `0` is isometric, `1` is top XY, `2` is front XZ, `3` is right YZ, and `R` resets camera orientation, pan and zoom.

The projection selector switches between **Perspective** and **Orthographic**. The draw-mode selector offers **Shaded** (lit surfaces), **Wireframe** (solid visible edges and dashed hidden edges) and **Shaded + Wire** (shaded surfaces with an edge overlay).

The axis gizmo in the upper-right corner indicates world X/Y/Z directions. The grid, world axes and labels can be toggled with `G`, `A` and `L`.

## 4. Selecting geometry

Choose **SELECT** (`S`) and click a vertex, edge or face. The Constructions panel reports the current selection. For a selected vertex or user point it displays coordinates; for an edge it displays its length.

Selection is based on screen-space proximity to projected vertices and edges and on the projected face polygon. It is intended for interactive inspection, not precision CAD constraints.

## 5. Creating points and constructions

### Points

1. Activate **POINT** (`P`).
2. Click in the viewport to create a point on the XY work plane.
3. By default, X and Y snap to increments of `0.5`. Toggle snapping in the Constructions tab or in **Settings → Interface and controls**.

To create a point at an arbitrary 3D position, enter X, Y and Z in the coordinate fields and use **Create point from XYZ**. Negative coordinates are supported. Created points appear in the point list and can be selected there.

Use **Delete** or **Delete point** to remove the selected user point. **Clear points** removes all user points. **Clear lines** removes all constructed segments, lines and rays. These actions do not modify the parametric model.

### Segments, lines and rays

Activate **SEGMENT** (`T`), **LINE** (`N`) or **RAY** (`Y`), then click two positions on the XY work plane. A segment joins the endpoints; a line extends in both directions; a ray starts at the first point and extends through the second.

The current tools do not provide snapping to existing model vertices or arbitrary 3D endpoint placement. Use explicit XYZ point entry for points outside XY.

## 6. Plane sections

Open the **Constructions** tab:

1. Select `XY / Z = d`, `XZ / Y = d`, `YZ / X = d` or `Custom / n·x = d`.
2. Set the offset `d`.
3. For a custom plane, enter normal components `nx`, `ny` and `nz`.
4. Enable **Show plane** and **Show section contour** as needed.

The plane equation is:

\[
n_x x+n_y y+n_z z=d
\]

You can also create three user points and choose **Plane through first 3 points**. The points must not be collinear. The application derives a normal from their cross product and sets the plane offset through the first point.

When the mesh intersects the plane in a polygon, the panel reports the number of section vertices, area `S` and perimeter `P`. The section is rendered as a highlighted foreground overlay so it remains visible on the model. The cutting plane itself is shown as a subtle, object-fitted sheet.

**Important:** this is an intersection visualization, not a Boolean solid operation. The original mesh remains intact; the application does not remove the part of the model on either side of the plane. Curved objects such as spheres and cylinders are represented by polygonal meshes, so their displayed sections are mesh approximations.

## 7. Vector analysis

Enter coordinates for points `A` and `B` in the vector section. The application calculates `AB = B − A`, magnitude `|AB|`, midpoint `M = (A + B) / 2` and angles between the vector and the positive X, Y and Z axes.

Enable **Show vector AB** to draw the vector with an arrow in the viewport. If A and B coincide, the vector has zero length and its direction angles are undefined.

## 8. Unit-circle lab

The Trigonometry tab contains an interactive unit circle. Change the angle with the slider or numeric field:

- Slider range: −720° to +720°.
- Numeric input range: −36000° to +36000°.
- Negative angles and multiple full turns are supported.

The circle shows the equivalent angular position, while the readout preserves the entered angle. The panel displays `P = (cos θ, sin θ)`, axis projections and trigonometric values. Recognized common angles are represented exactly using π, for example `π/6`, `π/4`, `π/3`, `π/2`, `π` and `2π`.

## 9. Settings and keyboard focus

Open **Settings → Interface and controls** to configure grid visibility, world axes, vertex and point labels, coordinate labels, snapping of new points to the `0.5` grid and orbit sensitivity.

Keyboard shortcuts are ignored while a text or numeric input has focus, so typing values does not accidentally switch tools.

## 10. Keyboard reference

| Key | Action |
|---|---|
| `0` / `1` / `2` / `3` | Isometric / top XY / front XZ / right YZ |
| `G` / `A` / `L` | Toggle grid / axes / labels |
| `O` / `S` / `P` | Navigation / select / point |
| `T` / `N` / `Y` | Segment / line / ray |
| `R` | Reset camera |
| `Delete` | Delete selected user point |
| `Esc` | Return to navigation tool |

## 11. State and persistence

The current application does not include project save/load or undo/redo. Model parameters, user points, constructions and view state belong to the running session and are not written to a project file.
