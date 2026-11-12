#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

void mouseMovement(int x, int y);
void updateCameraDirection();
void setupPlane();
void setupSkybox();

GLuint planeVBO, skyboxVBO;
int planeVertexCount;

// player spawns at the center of the map
float camX = 0.0f;
float camY = 2.0f;
float camZ = 0.0f;

float yaw = -90.0f;
float pitch = 0.0f;

float sensitivity = 0.1f;

int lastMouseX = 400;
int lastMouseY = 300;

bool wPressed = false, aPressed = false, sPressed = false, dPressed = false;
bool firstMouse = true;

float frontX, frontY, frontZ;

void mouseMovement(int x, int y)
{
    int centerX = glutGet(GLUT_WINDOW_WIDTH) / 2;
    int centerY = glutGet(GLUT_WINDOW_HEIGHT) / 2;

    if (firstMouse)
    {
        glutWarpPointer(centerX, centerY);
        firstMouse = false;
        return;
    }

    float offsetX = x - centerX;
    float offsetY = centerY - y;

    yaw += offsetX * sensitivity;
    pitch += offsetY * sensitivity;

    if (pitch > 89.0f)
        pitch = 89.0f;

    if (pitch < -89.0f)
        pitch = -89.0f;

    updateCameraDirection();

    glutWarpPointer(centerX, centerY);

    glutPostRedisplay();
}

void updateCameraDirection()
{
    frontX = cos(yaw * 3.14159265f / 180.0f) *
             cos(pitch * 3.14159265f / 180.0f);

    frontY = sin(pitch * 3.14159265f / 180.0f);

    frontZ = sin(yaw * 3.14159265f / 180.0f) *
             cos(pitch * 3.14159265f / 180.0f);
}

void setupSkybox()
{
  float size = 50.0f;

  GLfloat skyboxVertices[] = {
      -size,  size, -size,
      -size, -size, -size,
       size, -size, -size,
       size, -size, -size,
       size,  size, -size,
      -size,  size, -size,

      -size, -size,  size,
      -size, -size, -size,
      -size,  size, -size,
      -size,  size, -size,
      -size,  size,  size,
      -size, -size,  size,

       size, -size, -size,
       size, -size,  size,
       size,  size,  size,
       size,  size,  size,
       size,  size, -size,
       size, -size, -size,

      -size, -size,  size,
      -size,  size,  size,
       size,  size,  size,
       size,  size,  size,
       size, -size,  size,
      -size, -size,  size,

      -size,  size, -size,
       size,  size, -size,
       size,  size,  size,
       size,  size,  size,
      -size,  size,  size,
      -size,  size, -size,

      -size, -size, -size,
      -size, -size,  size,
       size, -size,  size,
       size, -size,  size,
       size, -size, -size,
      -size, -size, -size
    };

    glGenBuffers(1, &skyboxVBO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(skyboxVertices),
        skyboxVertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawSkybox()
{   
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    if (!glIsBuffer(skyboxVBO))
    {
        std::cout << "Invalid Skybox VBO!" << std::endl;
        return;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(3, GL_FLOAT, 0, (void*)0);

      glColor3ub(61, 66, 85); // Back Face
      glDrawArrays(GL_TRIANGLES, 0, 6);

      glColor3ub(28, 35, 53); // Left Face
      glDrawArrays(GL_TRIANGLES, 6, 6);

      glColor3ub(20, 25, 44); // Right Face
      glDrawArrays(GL_TRIANGLES, 12, 6);

      glColor3ub(20, 25, 44); // Front Face
      glDrawArrays(GL_TRIANGLES, 18, 6);

      glColor3ub(19, 27, 40); // Top Face
      glDrawArrays(GL_TRIANGLES, 24, 6);

      glColor3ub(0, 0, 0); // Bottom Face
      glDrawArrays(GL_TRIANGLES, 30, 6);

    glDisableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void setupPlane()
{
    std::vector<float> vertices;

    for (int i = -20; i <= 20; i++)
    {
        vertices.push_back(i);
        vertices.push_back(0.0f);
        vertices.push_back(-20.0f);

        vertices.push_back(i);
        vertices.push_back(0.0f);
        vertices.push_back(20.0f);

        vertices.push_back(-20.0f);
        vertices.push_back(0.0f);
        vertices.push_back(i);

        vertices.push_back(20.0f);
        vertices.push_back(0.0f);
        vertices.push_back(i);
    }

    planeVertexCount = vertices.size() / 3;

    glGenBuffers(1, &planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawPlane()
{
    glColor3f(0.5f, 0.5f, 0.5f);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);

    if (!glIsBuffer(planeVBO))
    {
        std::cout << "Invalid Plane VBO!" << std::endl;
        return;
    }

    glEnableClientState(GL_VERTEX_ARRAY);

      glVertexPointer(3, GL_FLOAT, 0, (void*)0);
      glDrawArrays(GL_LINES, 0, planeVertexCount);

    glDisableClientState(GL_VERTEX_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ====================================================
// CAMPSITE (floor plan only, placeholder shapes)
//
// You start at (0, 0, 0) looking toward -Z.
//
//   z = -20   cliff edge
//   z = -5    campfire
//   z = -3    left tent (x = -8)    right tent (x = 8)
//   z = 0     player spawn
//   z = 15    entrance sign
//
// The clearing is the square from -12 to 12 in x and z.
// Trees are placed randomly outside of the clearing.
// ====================================================

GLuint cubeVBO, pyramidVBO, prismVBO;

// time in seconds, used for the campfire animation
float fireTime = 0.0f;

// settings for the random trees
const int MAX_TREES = 30; // number of trees
float treeX[MAX_TREES];
float treeZ[MAX_TREES];
float treeScale[MAX_TREES];
int treeCount = 0;
int treeSeed = 25; // change this number to get a different forest

// cube: x and z go from -0.5 to 0.5, y goes from 0 to 1
void setupCube()
{
    float cubeVertices[] = {
        // back
        -0.5f, 1.0f, -0.5f,  -0.5f, 0.0f, -0.5f,   0.5f, 0.0f, -0.5f,
         0.5f, 0.0f, -0.5f,   0.5f, 1.0f, -0.5f,  -0.5f, 1.0f, -0.5f,
        // left
        -0.5f, 0.0f,  0.5f,  -0.5f, 0.0f, -0.5f,  -0.5f, 1.0f, -0.5f,
        -0.5f, 1.0f, -0.5f,  -0.5f, 1.0f,  0.5f,  -0.5f, 0.0f,  0.5f,
        // right
         0.5f, 0.0f, -0.5f,   0.5f, 0.0f,  0.5f,   0.5f, 1.0f,  0.5f,
         0.5f, 1.0f,  0.5f,   0.5f, 1.0f, -0.5f,   0.5f, 0.0f, -0.5f,
        // front
        -0.5f, 0.0f,  0.5f,  -0.5f, 1.0f,  0.5f,   0.5f, 1.0f,  0.5f,
         0.5f, 1.0f,  0.5f,   0.5f, 0.0f,  0.5f,  -0.5f, 0.0f,  0.5f,
        // top
        -0.5f, 1.0f, -0.5f,   0.5f, 1.0f, -0.5f,   0.5f, 1.0f,  0.5f,
         0.5f, 1.0f,  0.5f,  -0.5f, 1.0f,  0.5f,  -0.5f, 1.0f, -0.5f
    };

    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// pyramid: square base on the ground, tip at y = 1 (4 sides only)
void setupPyramid()
{
    float pyramidVertices[] = {
        -0.5f, 0.0f, -0.5f,   0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f, 0.0f, -0.5f,   0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  -0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f, 0.0f,  0.5f,  -0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f
    };

    glGenBuffers(1, &pyramidVBO);
    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// prism (tent shape): the ridge runs along Z, the front triangle is at +Z
void setupPrism()
{
    float prismVertices[] = {
        // left slope
        -0.5f, 0.0f, -0.5f,  -0.5f, 0.0f,  0.5f,   0.0f, 1.0f,  0.5f,
         0.0f, 1.0f,  0.5f,   0.0f, 1.0f, -0.5f,  -0.5f, 0.0f, -0.5f,
        // right slope
         0.5f, 0.0f,  0.5f,   0.5f, 0.0f, -0.5f,   0.0f, 1.0f, -0.5f,
         0.0f, 1.0f, -0.5f,   0.0f, 1.0f,  0.5f,   0.5f, 0.0f,  0.5f,
        // front triangle
        -0.5f, 0.0f,  0.5f,   0.5f, 0.0f,  0.5f,   0.0f, 1.0f,  0.5f,
        // back triangle
         0.5f, 0.0f, -0.5f,  -0.5f, 0.0f, -0.5f,   0.0f, 1.0f, -0.5f
    };

    glGenBuffers(1, &prismVBO);
    glBindBuffer(GL_ARRAY_BUFFER, prismVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(prismVertices), prismVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// random number between minValue and maxValue
float randomRange(float minValue, float maxValue)
{
    float t = rand() / (float)RAND_MAX;
    return minValue + t * (maxValue - minValue);
}

// picks random spots for the trees (runs once at the start)
void setupTrees() //claude
{
    srand(treeSeed);

    int tries = 0;

    while (treeCount < MAX_TREES && tries < 5000)
    {
        tries++;

        float x = randomRange(-19.0f, 19.0f);
        float z = randomRange(-19.0f, 19.0f);

        // not inside the clearing
        if (fabs(x) < 12.0f && fabs(z) < 12.0f)
        {
            continue;
        }

        // keep the cliff side open so the view is not blocked
        if (z < -15.0f)
        {
            continue;
        }

        // keep the entrance path free
        if (fabs(x) < 4.5f && z > 12.0f)
        {
            continue;
        }

        // not too close to another tree
        bool tooClose = false;

        for (int i = 0; i < treeCount; i++)
        {
            float dx = x - treeX[i];
            float dz = z - treeZ[i];

            if (dx * dx + dz * dz < 9.0f)
            {
                tooClose = true;
            }
        }

        if (tooClose)
        {
            continue;
        }

        treeX[treeCount] = x;
        treeZ[treeCount] = z;
        treeScale[treeCount] = randomRange(0.8f, 1.3f);
        treeCount++;
    }
}

void setupCampsite()
{
    setupCube();
    setupPyramid();
    setupPrism();
    setupTrees();
}

// x, y, z is the bottom center of the cube
void drawCube(float x, float y, float z, float sx, float sy, float sz,
              int r, int g, int b)
{
    float red = r / 255.0f;
    float green = g / 255.0f;
    float blue = b / 255.0f;

    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(sx, sy, sz);

        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glEnableClientState(GL_VERTEX_ARRAY);
          glVertexPointer(3, GL_FLOAT, 0, (void*)0);

          // back and front a little darker
          glColor3f(red * 0.8f, green * 0.8f, blue * 0.8f);
          glDrawArrays(GL_TRIANGLES, 0, 6);
          glDrawArrays(GL_TRIANGLES, 18, 6);

          // left and right darkest
          glColor3f(red * 0.6f, green * 0.6f, blue * 0.6f);
          glDrawArrays(GL_TRIANGLES, 6, 12);

          // top is the normal color
          glColor3f(red, green, blue);
          glDrawArrays(GL_TRIANGLES, 24, 6);

        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopMatrix();
}

void drawPyramid(float x, float y, float z, float sx, float sy, float sz,
                 int r, int g, int b)
{
    float red = r / 255.0f;
    float green = g / 255.0f;
    float blue = b / 255.0f;

    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(sx, sy, sz);

        glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
        glEnableClientState(GL_VERTEX_ARRAY);
          glVertexPointer(3, GL_FLOAT, 0, (void*)0);

          // light sides and dark sides
          glColor3f(red * 0.8f, green * 0.8f, blue * 0.8f);
          glDrawArrays(GL_TRIANGLES, 0, 3);
          glDrawArrays(GL_TRIANGLES, 6, 3);

          glColor3f(red * 0.6f, green * 0.6f, blue * 0.6f);
          glDrawArrays(GL_TRIANGLES, 3, 3);
          glDrawArrays(GL_TRIANGLES, 9, 3);

        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopMatrix();
}

// x, y, z is the bottom center of the prism
void drawPrism(float x, float y, float z, float sx, float sy, float sz,
               int r, int g, int b)
{
    float red = r / 255.0f;
    float green = g / 255.0f;
    float blue = b / 255.0f;

    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(sx, sy, sz);

        glBindBuffer(GL_ARRAY_BUFFER, prismVBO);
        glEnableClientState(GL_VERTEX_ARRAY);
          glVertexPointer(3, GL_FLOAT, 0, (void*)0);

          // left slope
          glColor3f(red * 0.9f, green * 0.9f, blue * 0.9f);
          glDrawArrays(GL_TRIANGLES, 0, 6);

          // right slope (the brightest one)
          glColor3f(red, green, blue);
          glDrawArrays(GL_TRIANGLES, 6, 6);

          // front triangle
          glColor3f(red * 0.85f, green * 0.85f, blue * 0.85f);
          glDrawArrays(GL_TRIANGLES, 12, 3);

          // back triangle
          glColor3f(red * 0.7f, green * 0.7f, blue * 0.7f);
          glDrawArrays(GL_TRIANGLES, 15, 3);

        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopMatrix();
}

// green grass for the whole plane, with a lighter green clearing in the middle
void drawGround()
{
    // grass
    drawCube(0.0f, -0.03f, 0.0f, 40.0f, 0.015f, 40.0f, 90, 160, 70);

    // clearing around the spawn
    drawCube(0.0f, -0.02f, 0.0f, 24.0f, 0.015f, 24.0f, 115, 180, 90);
}

// the ground ends at z = -20, with a big drop after it
void drawCliff()
{
    // rock edge
    drawCube(0.0f, -0.2f, -20.2f, 40.0f, 0.2f, 0.4f, 110, 95, 70);

    // ground far below
    drawCube(0.0f, -26.0f, -33.0f, 80.0f, 1.0f, 26.0f, 28, 60, 45);
}

// one flame piece. It grows and shrinks with the time (sin goes up and down)
void drawFlame(float x, float y, float z, float size, float speed, float phase,
               int r, int g, int b)
{
    float s = 0.65f + 0.35f * sin(fireTime * speed + phase);

    drawPyramid(x, y, z, size * s * 0.7f, size * s, size * s * 0.7f, r, g, b);
}

void drawCampfire(float x, float z)
{
    // 4 stones around the fire
    drawCube(x + 0.8f, 0.0f, z, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x - 0.8f, 0.0f, z, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x, 0.0f, z + 0.8f, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x, 0.0f, z - 0.8f, 0.4f, 0.3f, 0.4f, 120, 120, 125);

    // log
    drawCube(x, 0.0f, z, 1.2f, 0.25f, 0.25f, 90, 55, 30);

    // 3 flames (each one has a different speed and phase so they don't move together)
    drawFlame(x, 0.25f, z, 1.0f, 6.0f, 0.0f, 255, 120, 30);
    drawFlame(x + 0.25f, 0.25f, z, 0.6f, 8.0f, 2.0f, 230, 70, 20);
    drawFlame(x - 0.2f, 0.25f, z, 0.5f, 10.0f, 4.0f, 255, 210, 60);
}

// A-frame tent. angle turns the tent so the door faces the campfire
void drawTent(float x, float z, float angle, int r, int g, int b)
{
    float width = 3.6f;
    float height = 2.6f;
    float length = 4.6f;

    glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);

        // main tent
        drawPrism(0.0f, 0.0f, 0.0f, width, height, length, r, g, b);

        // door (brown triangle on the front)
        drawPrism(0.0f, 0.0f, length / 2 + 0.02f, width * 0.45f, height * 0.65f, 0.05f,
                  185, 140, 80);
    glPopMatrix();
}

void drawPineTree(float x, float z, float s)
{
    // trunk
    drawCube(x, 0.0f, z, 0.4f * s, 1.3f * s, 0.4f * s, 85, 55, 30);

    // leaves
    drawPyramid(x, 1.0f * s, z, 2.6f * s, 2.0f * s, 2.6f * s, 30, 90, 50);
    drawPyramid(x, 2.4f * s, z, 2.0f * s, 1.8f * s, 2.0f * s, 36, 105, 58);
}

void drawRock(float x, float z, float size)
{
    drawPyramid(x, 0.0f, z, size * 1.4f, size, size * 1.2f, 110, 110, 115);
}

void drawLantern(float x, float z)
{
    // post
    drawCube(x, 0.0f, z, 0.1f, 1.6f, 0.1f, 70, 45, 25);

    // light
    drawCube(x, 1.6f, z, 0.3f, 0.4f, 0.3f, 255, 215, 100);
}

void drawEntranceSign(float x, float z)
{
    // two posts
    drawCube(x - 1.8f, 0.0f, z, 0.25f, 2.6f, 0.25f, 95, 62, 35);
    drawCube(x + 1.8f, 0.0f, z, 0.25f, 2.6f, 0.25f, 95, 62, 35);

    // board
    drawCube(x, 1.5f, z, 3.8f, 0.9f, 0.15f, 140, 95, 55);
}

// draws all the random trees from setupTrees()
void drawTrees()
{
    for (int i = 0; i < treeCount; i++)
    {
        drawPineTree(treeX[i], treeZ[i], treeScale[i]);
    }
}

void drawRocks()
{
    drawRock(-15.0f, -8.0f, 1.0f);
    drawRock(-15.0f, 4.0f, 1.3f);
    drawRock(15.0f, -9.0f, 1.1f);
    drawRock(15.0f, 2.0f, 1.4f);
    drawRock(-10.0f, -18.0f, 1.2f);
    drawRock(11.0f, -18.0f, 1.0f);
}

void drawLanterns()
{
    drawLantern(-4.5f, -3.0f);
    drawLantern(4.5f, -3.0f);
    drawLantern(-2.0f, -6.0f);
    drawLantern(2.0f, -6.0f);
    drawLantern(-3.0f, 15.0f);
    drawLantern(3.0f, 15.0f);
}

void drawCampsite()
{
    drawGround();
    drawCliff();

    drawCampfire(0.0f, -5.0f);

    // the angles turn the doors toward the campfire
    drawTent(-8.0f, -3.0f, 104.0f, 253, 208, 120);
    drawTent(8.0f, -3.0f, -104.0f, 245, 185, 100);

    drawEntranceSign(0.0f, 15.0f);

    drawLanterns();
    drawTrees();
    drawRocks();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(
        camX, camY, camZ,
        camX + frontX, camY + frontY, camZ + frontZ,
        0.0f, 1.0f, 0.0f
    );

    glPushMatrix();
        glTranslatef(camX, camY, camZ);
        drawSkybox();
    glPopMatrix();

    drawPlane();

    drawCampsite();

    glutSwapBuffers();
}

void movementLogic()
{
    float speed = 0.001f;

    float rightX = -sin(yaw * 3.14159265f / 180.0f);
    float rightZ = cos(yaw * 3.14159265f / 180.0f);

    if (wPressed)
    {
        camX += frontX * speed;
        camZ += frontZ * speed;
    }

    if (sPressed)
    {
        camX -= frontX * speed;
        camZ -= frontZ * speed;
    }

    if (aPressed)
    {
        camX -= rightX * speed;
        camZ -= rightZ * speed;
    }

    if (dPressed)
    {
        camX += rightX * speed;
        camZ += rightZ * speed;
    }
}

void keyboardPress(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'w':
            wPressed = true;
            break;

        case 's':
            sPressed = true;
            break;

        case 'a':
            aPressed = true;
            break;

        case 'd':
            dPressed = true;
            break;

        case 27:
            exit(0);
    }

    glutPostRedisplay();
}

void keyboardRelease(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'w':
            wPressed = false;
            break;

        case 's':
            sPressed = false;
            break;

        case 'a':
            aPressed = false;
            break;

        case 'd':
            dPressed = false;
            break;
    }

    glutPostRedisplay();
}

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0, (float)w / h, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
}

void setupLogic()
{
  setupPlane();
  setupSkybox();
  setupCampsite();
}

void sceneLogic()
{
    // time in seconds (used by the campfire flames)
    fireTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    movementLogic();
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("3D Free Look Environment");

    GLenum err = glewInit();

    glutFullScreen();
    glEnable(GL_DEPTH_TEST);

    setupLogic();
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(400, 300);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardPress);
    glutKeyboardUpFunc(keyboardRelease);
    glutIdleFunc(sceneLogic);
    glutPassiveMotionFunc(mouseMovement);

    updateCameraDirection();

    if (err == GLEW_OK)
    {
        std::cout << "GLEW initialized successfully." << std::endl;
        std::cout << "Using GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;  
        glutMainLoop();
    }
    else {
        std::cout << "GLEW initialization failed: "
                  << glewGetErrorString(err) << std::endl;

        return 1;
    }

    return 0;
}