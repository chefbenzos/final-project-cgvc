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

GLuint planeVBO;
int planeVertexCount;

float camX = 0.0f;
float camY = 2.0f;
float camZ = 5.0f;

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

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(
        camX, camY, camZ,
        camX + frontX, camY + frontY, camZ + frontZ,
        0.0f, 1.0f, 0.0f
    );

    drawPlane();

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
        camY += frontY * speed;
        camZ += frontZ * speed;
    }

    if (sPressed)
    {
        camX -= frontX * speed;
        camY -= frontY * speed;
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

void sceneLogic()
{
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

    setupPlane();
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
