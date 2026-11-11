# Night Sky Meteors 

A small OpenGL project that recreates the glowing comet scene from *Your Name*. It shows a dark starry sky, blue-gray clouds on the right, a big colorful comet, and a smaller pink meteor.

This project was made for an introductory OpenGL course, so it only uses **C++, GLUT/FreeGLUT, and fixed-function OpenGL**. There are no shaders and no extra libraries.

## What You Will See

- A navy-blue sky with many stars, and some of them twinkle
- Clouds on the right side that drift slowly
- A big comet with a blue, cyan, purple, and magenta tail and a pulsing glowing head
- A small pink meteor below the big comet
- Both meteors restart when they leave the screen

## Controls

| Key     | What it does                    |
|---------|---------------------------------|
| `SPACE` | Pause / resume the animation    |
| `R`     | Reset the meteors and animation |
| `ESC`   | Close the program               |


## Main Functions

- `initializeOpenGL()` sets up OpenGL and calls the other setup functions
- `setupLighting()` and `setupTextures()` prepare lights and textures
- `createStars()`, `createClouds()`, `createMeteorGeometry()` build the scene data
- `drawStars()`, `drawClouds()`, `drawMainMeteor()`, `drawSmallMeteor()` draw each part
- `updateAnimation()` moves everything using the elapsed time
- `display()`, `reshape()`, `keyboard()` are the GLUT callbacks

## Tips for Changing the Scene

- **Tail colors:** edit the `layers[]` table in `createMeteorGeometry()`
- **Cloud positions:** edit the `addCloud(...)` lines in `createClouds()`
- **Comet speed or direction:** change `MAIN_SPEED` at the top of the file (a negative number moves the head down-left, like in the film; a positive number moves it up-right)

## Notes

- The window is 16:9 (1280 x 720). If you resize it, black bars keep the picture from stretching.
- No image files are needed. All textures are created inside the program.