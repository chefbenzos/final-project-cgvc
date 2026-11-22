# Night Campsite with a Meteor Sky

## Overview

Night Campsite with a Meteor Sky is a 3D interactive environment developed using C++, OpenGL, and FreeGLUT. The project features a campsite surrounded by a forest under an animated night sky. The environment includes a meteor shower, animated clouds, twinkling stars, a campsite with tents and a campfire, and a dynamic explosion sequence.

The program allows users to explore the environment using first-person camera controls while observing the animated sky and campsite.

## Features

- **3D Environment** – A campsite featuring tents, trees, rocks, lanterns, a campfire, and textured ground.
- **First-Person Camera** – Allows users to explore the environment using keyboard and mouse controls.
- **Animated Night Sky** – Includes a gradient sky, twinkling stars, moving clouds, and multiple meteors.
- **Meteor Animation** – Features a main meteor traveling across the sky and a smaller meteor that separates and approaches a landing point.
- **Explosion Sequence** – Includes an expanding explosion, a ground-level shockwave, and a screen-wide flash.
- **Dynamic Lighting** – Multiple light sources illuminate the environment, including meteor lights, campfire lighting, and lanterns.
- **Procedural Generation** – Generates stars, clouds, and additional trees using randomized values.
- **OpenGL Vertex Buffer Objects (VBOs)** – Uses VBOs to store and render 3D shapes.
- **Texture Generation** – Generates a grass texture programmatically without requiring an external image file.

## Controls

| Key / Input | Action |
|---|---|
| W | Move forward |
| A | Move left |
| S | Move backward |
| D | Move right |
| Mouse | Look around |
| Spacebar | Pause or resume the animation timeline |
| R | Restart the timeline |
| C | Reset the camera position and orientation |
| [ | Move the timeline backward by one second |
| ] | Move the timeline forward by one second |
| F | Toggle fullscreen |
| ESC | Exit the program |

## Technologies Used

- C++
- OpenGL
- GLEW
- FreeGLUT
- OpenGL Utility Library (GLU)

## Graphics Concepts Implemented

The project applies several fundamental computer graphics concepts:

- 3D geometric transformations using translation, rotation, and scaling
- Perspective projection
- Camera positioning using `gluLookAt()`
- Vertex Buffer Objects (VBOs)
- Vertex arrays
- Lighting and material properties
- Texture mapping
- Primitive rendering using triangles, quads, lines, points, and triangle fans
- Animation using elapsed time
- Mouse-based camera rotation
- Depth testing
- Matrix stack operations

## Environment and Animation

The environment is divided into two main components:

### Night Sky

The night sky is rendered independently from the campsite. It contains a gradient background, stars, clouds, and meteors.

The stars continuously change brightness to simulate twinkling. Clouds slowly move across the sky, while the main meteor travels along a curved path. A smaller meteor separates from the main meteor and gradually approaches a predetermined landing point.

### Campsite

The campsite contains a textured ground, trees, tents, rocks, lanterns, and a campfire. Lighting is updated throughout the animation to simulate the changing illumination caused by the meteors and campfire.

When the smaller meteor reaches its landing point, an explosion sequence begins. The explosion expands, producing a bright flash that temporarily turns the scene white before the animation timeline restarts.

## Installation and Setup

1. Install a C++ compiler compatible with your development environment.
2. Install OpenGL, GLEW, FreeGLUT, and GLU.
3. Configure your IDE or compiler to include the required OpenGL libraries and headers.
4. Open the project source file `main.cpp`.
5. Compile the program using your configured development environment.
6. Run the compiled executable.

## Project Structure

```text
Night-Campsite/
│
├── main.cpp
└── README.md
```

## Notes

- The project uses legacy fixed-function OpenGL rendering.
- The grass texture is generated programmatically.
- The stars, clouds, and additional trees are generated using random values.
- The animation timeline runs for approximately 20 seconds before restarting.
- The camera can be moved and rotated independently of the animation timeline.

## Author

Developed as a computer graphics project using C++, OpenGL, and FreeGLUT.
