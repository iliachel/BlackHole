# BlackHole

`BlackHole` is a Qt/OpenGL prototype for visualizing a Kerr black hole and gradually moving the project from a purely artistic shader demo toward a more physically grounded simulator.

## Current state

The project now has two distinct layers:

1. A physics layer for Kerr spacetime geodesics.
2. A realtime visualization layer that renders a black hole, photon ring glow, background stars, and an emissive accretion disk.

The physics layer is more serious than the original prototype, but the renderer is still approximate. In other words:

- the project is no longer just a "pretty fake" shader,
- but it is not yet a full physically correct Kerr ray tracer.

## What was changed

### 1. Physics foundation

The new files in `physics/` introduce the basis for a real Kerr geodesic solver:

- `physics/kerrspacetime.h`
- `physics/kerrspacetime.cpp`
- `physics/geodesicintegrator.h`
- `physics/geodesicintegrator.cpp`

These files implement:

- a Boyer-Lindquist state,
- components of the contravariant Kerr metric,
- a Hamiltonian formulation,
- numerical derivatives of the Hamiltonian,
- RK4 integration for geodesics.

This gives the project a reusable physics core that is independent from the Qt widget.

### 2. Renderer cleanup

The old particle stripe was removed from the scene. The image is now driven by a fullscreen fragment shader in `glwidget.cpp`, which does the following:

- emits camera rays from the viewer,
- bends them using the current approximate gravitational model,
- accumulates emissive disk light along the path,
- adds a horizon glow and star background,
- tone-maps the result before displaying it.

This is still a visual approximation, but it is much cleaner than overlaying an unrelated particle band on top of the black hole.

### 3. Orbit camera

The camera was changed from a loose FPS-style camera to an orbital camera around the black hole center.

That means:

- the black hole remains the focus of the scene,
- the viewer moves around it instead of drifting away,
- orbit controls are easier for inspection and presentation.

## Controls

- `W` / `S`: zoom in / zoom out
- `A` / `D`: orbit left / right
- `Q` / `E`: orbit up / down
- `Shift`: faster movement
- Hold right mouse button and move mouse: rotate orbit manually

## File overview

- `main.cpp`: Qt application entry point and OpenGL format setup
- `mainwindow.cpp`: main window and central widget setup
- `glwidget.h`, `glwidget.cpp`: realtime renderer, camera control, shader setup
- `camera.h`: simple camera basis and view helpers
- `physics/`: Kerr spacetime and geodesic integration core
- `shaders/physics.comp`: legacy compute shader from the earlier prototype

## How it works

### Camera

`GLWidget` stores orbital parameters:

- `orbitDistance`
- `orbitAzimuth`
- `orbitElevation`

Each frame, these values are converted into a Cartesian camera position around the origin, which is treated as the black hole center. The camera yaw/pitch are then updated so the camera always looks back toward the center.

### Black hole rendering

The fragment shader receives:

- camera position,
- camera basis,
- black hole mass `M`,
- spin `a`,
- field of view and aspect ratio.

For each screen pixel it:

1. creates a ray,
2. advances the ray step by step,
3. applies approximate gravitational bending,
4. accumulates emission from the accretion disk,
5. stops if the ray crosses the horizon,
6. otherwise blends in the background star field.

### Physics diagnostics

At startup, the app also runs a sample geodesic through the new Kerr physics layer and prints a short diagnostic summary:

- mass,
- spin,
- horizon radius,
- initial Hamiltonian,
- Hamiltonian drift.

This does not yet drive the visual ray integration directly, but it confirms that the new physics module is wired into the app.

## Limitations

The current renderer still uses an approximate bending model in the fragment shader. The physically correct next step would be:

1. use null geodesics from the Kerr solver for light propagation,
2. intersect those rays with a proper accretion disk model,
3. add relativistic beaming and redshift in a consistent way.

So this repository is currently best described as:

"a Kerr black hole visualization prototype with an emerging real physics core."

## Build note

The source tree was updated, but on this machine CMake currently fails before full compilation because the configured MinGW toolchain does not pass CMake's test compile step. That issue is external to the source changes and needs to be fixed in the local Qt/MinGW environment before a clean build can be verified.
