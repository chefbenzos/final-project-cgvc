#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace std;

//  CONSTANTS
const float PI         = 3.14159265f;
const float EYE_HEIGHT = 2.0f;

// ---- the timeline (seconds) ----
const float DURATION   = 20.0f;   // length of the whole sequence
const float T_SEP      = 4.0f;    // the small meteor leaves the main meteor
const float T_HIT      = 14.5f;   // the small meteor lands -> explosion starts
const float T_WHITE    = 17.5f;   // the screen is completely white
const float T_RESET    = 18.4f;   // the sky restarts (hidden by the white)
const float T_MAIN_END = 17.0f;   // the main meteor reaches the front horizon

// ---- the left tent and the landing point ----
// The tent is at (-8, -3), turned 104 degrees, so its door looks toward +X.
// The landing point is 2 m out of the door and 1.8 m to the far (north) side.
const float TENT_X = -8.0f, TENT_Z = -3.0f, TENT_ANGLE = 104.0f;
const float LANDING_X = -4.3f;
const float LANDING_Y =  0.05f;
const float LANDING_Z = -5.8f;

// ---- the main meteor moves on a half circle in a vertical plane ----
const float ARC_RADIUS = 44.0f;                   // metres around the player
const float PLANE_X = 0.62f, PLANE_Z = 0.785f;    // direction of the plane (toward the rear)
const int   TAIL_STEPS = 70;                      // number of balls in the tail
const float TAIL_STEP_DEG = 1.0f;                 // angle between two tail balls

// ---- stars, clouds, fast meteors ----
const int NUM_SMALL = 1800;
const int NUM_BIG   = 250;
const int NUM_STARS = NUM_SMALL + NUM_BIG;
const int NUM_CLOUDS = 18;
const int NUM_EXTRA  = 6;

// time
float sceneTime = 0.0f;       // the global timeline (0 .. DURATION)
float skyTime   = 0.0f;       // the time used by the sky objects (see updateGlobalSceneTime)
float flash     = 0.0f;       // 0 = normal night, 1 = everything white
float fireTime  = 0.0f;       // real time, only for the flickering campfire
bool  paused    = false;
int   lastMs    = 0;

// camera
float camX = 0.0f, camZ = 0.0f;
float yaw = -90.0f, pitch = 0.0f;
int   winW = 1280, winH = 720;
bool  fullscreen = false;
bool  firstMouse = true;
float sensitivity = 0.1f;
float frontX, frontY, frontZ;

// movement borders (the player cannot leave the ground area)
float minX = -20.0f;
float maxX =  20.0f;
float minZ = -20.0f;
float maxZ =  20.0f;


// movement
bool wPressed = false, aPressed = false, sPressed = false, dPressed = false;

// VBO ids (Module 5)
GLuint cubeVBO, pyramidVBO, prismVBO;

// texture id (Module 8)
GLuint grassTexture;

// stars (vertex arrays, Module 4)
float starPos[NUM_STARS * 3];      // x y z
float starCol[NUM_STARS * 3];      // r g b  (changes when the star twinkles)
float starTint[NUM_STARS * 3];     // the normal colour of the star
float starBright[NUM_STARS];
float starPhase[NUM_STARS];

// clouds
float cloudX[NUM_CLOUDS], cloudY[NUM_CLOUDS], cloudZ[NUM_CLOUDS];
float cloudAz[NUM_CLOUDS], cloudScale[NUM_CLOUDS];

// fast meteors: start point, end point, how often they fly, colour
float exStart[NUM_EXTRA][3] = {
    {  70, 90, -60 }, { -80, 80, -30 }, { -50, 90, 70 },
    {  80, 75,  40 }, {  10, 100, -80 }, {  60, 85, 75 } };
float exEnd[NUM_EXTRA][3] = {
    { -20, 45, -75 }, { -70, 35,  40 }, {  30, 40, 85 },
    {  85, 30, -30 }, { -60, 55, -50 }, { -10, 45, 90 } };
float exPeriod[NUM_EXTRA] = { 3.0f, 3.6f, 4.2f, 3.3f, 2.8f, 3.9f };
float exPhase[NUM_EXTRA]  = { 0.0f, 0.3f, 0.6f, 0.15f, 0.8f, 0.5f };
float exColor[NUM_EXTRA][3] = {
    { 0.75f, 0.88f, 1.0f }, { 0.70f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f },
    { 0.85f, 0.75f, 1.0f }, { 0.90f, 0.95f, 1.0f }, { 0.75f, 0.85f, 1.0f } };

// The first 12 trees are the original fixed ones. The rest are random.
const int   GROUND_HALF    = 50;      // the ground goes from -50 to +50 in X and Z (100 m x 100 m)
const int   MAX_TREES      = 180;     // maximum number of trees
const float TREE_AREA      = 48.0f;   // trees stay inside +-48 m (the ground is +-50 m)
const float MIN_TREE_DIST  = 3.0f;    // minimum distance between two trees
int   treeSeed = 25;                  // change this number to get a different forest
float treeX[MAX_TREES];
float treeZ[MAX_TREES];
float treeScale[MAX_TREES];
int   treeCount = 0;

// normals of the faces of our shapes (used by the lighting)
float cubeNormals[6][3] = {
    { 0, 0, -1 }, { -1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, -1, 0 } };
float pyramidNormals[4][3] = {
    { 0, 0.5f, -1 }, { 1, 0.5f, 0 }, { 0, 0.5f, 1 }, { -1, 0.5f, 0 } };

// Scene Setup Functions
void initializeOpenGL();
void setupLighting();
void setupTextures();
void setupShapes();
void initializeStars();
void initializeClouds();
void setupTrees();

void updateGlobalSceneTime(float dt);
void updateStars();
void updateMeteorLighting();

// Scene Drawing Functions
void drawNightSky();
void drawStars();
void drawClouds();
void drawMainMeteor();
void drawDerailedMeteor();
void drawAdditionalMeteors();
void drawExplosion();
void drawGround();
void drawCampsite();
void drawTrees();
void drawHUD();

// Main Display Functions and Keyboard and Mouse Callbacks
void display();
void reshape(int w, int h);
void keyboardPress(unsigned char key, int x, int y);
void keyboardRelease(unsigned char key, int x, int y);
void movementLogic();
void mouse(int button, int state, int x, int y);
void mouseMovement(int x, int y);
void updateCameraDirection();
void timer(int value);

float randomRange(float minValue, float maxValue)
{
    float t = rand() / (float)RAND_MAX;
    return minValue + t * (maxValue - minValue);
}

float smoothstep(float a, float b, float x)
{
    float t = (x - a) / (b - a);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

// glColor, but mixed with white by the flash value.
// flash = 0 -> the normal colour, flash = 1 -> pure white.
void setColor(float r, float g, float b)
{
    glColor3f(r + (1.0f - r) * flash,
              g + (1.0f - g) * flash,
              b + (1.0f - b) * flash);
}
void setColor255(int r, int g, int b)
{
    setColor(r / 255.0f, g / 255.0f, b / 255.0f);
}

// a ball (used for the glow of the meteors)
void drawBall(float x, float y, float z, float radius)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidSphere(radius, 10, 8);
    glPopMatrix();
}

// a flat circle made with a triangle fan (used for the clouds)
void drawDisc(float cx, float cy, float radius)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx, cy, 0.0f);
    for (int i = 0; i <= 16; i++)
    {
        float a = 2.0f * PI * i / 16.0f;
        glVertex3f(cx + radius * cosf(a), cy + radius * sinf(a), 0.0f);
    }
    glEnd();
}

// ============================================================================
//  MAIN METEOR  -  position as a function of time
//  theta is the angle on the half circle:
//     0 degrees   = rear horizon
//    90 degrees   = straight overhead
//   180 degrees   = front horizon
// ============================================================================
void arcPoint(float theta, float p[3])
{
    float a = theta * PI / 180.0f;
    p[0] = ARC_RADIUS * cosf(a) * PLANE_X;
    p[1] = EYE_HEIGHT + ARC_RADIUS * sinf(a);
    p[2] = ARC_RADIUS * cosf(a) * PLANE_Z;
}

// the angle grows evenly: 0 at t = 0 and 180 at T_MAIN_END
float mainTheta(float t)
{
    float theta = 180.0f * t / T_MAIN_END;
    if (theta > 180.0f) theta = 180.0f;
    return theta;
}

// ============================================================================
//  SMALL (DERAILED) METEOR  -  position as a function of time
//    t < T_SEP        Stage 1: it flies next to the main meteor
//    T_SEP .. T_HIT   Stage 2: it slowly leaves the main meteor and curves
//                     down toward the landing point
//    t >= T_HIT       Stage 3: it is at the landing point (the explosion)
//  The curve is a "lerp" (linear interpolation) between two positions:
//     position = follower * (1 - w) + landing * w
//  where w goes from 0 to 1 with smoothstep. So the path is smooth and has
//  no loops and no spinning.
// ============================================================================
void derailedPos(float t, float p[3])
{
    float head[3];
    arcPoint(mainTheta(t), head);

    // the "follower" position: a little beside and below the main meteor head
    float fx = head[0] - 2.0f;
    float fy = head[1] + 1.0f;
    float fz = head[2] + 2.0f;

    float w = smoothstep(T_SEP, T_HIT, t);

    p[0] = fx * (1.0f - w) + LANDING_X * w;
    p[1] = fy * (1.0f - w) + LANDING_Y * w;
    p[2] = fz * (1.0f - w) + LANDING_Z * w;
}

// ============================================================================
//  INITIALIZATION
// ============================================================================
void initializeOpenGL()
{
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    srand(7);                  // fixed seed: the same stars and clouds every run

    setupLighting();
    setupTextures();
    setupShapes();
    initializeStars();
    initializeClouds();
    setupTrees();              // last: it calls srand() again, so it must come after the stars and clouds
}

// We use 6 lights:
//   LIGHT0  main meteor head      (point light, blue-white)
//   LIGHT1  campfire              (point light, orange, flickers, attenuation)
//   LIGHT2  small meteor          (point light, pink -> white)
//   LIGHT3  night sky             (directional light, faint violet-blue)
//   LIGHT4  explosion             (point light AT THE LANDING POINT)
//   LIGHT5  spot light on the front of the tent
// Their colours and positions are updated every frame in updateMeteorLighting().
void setupLighting()
{
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
    glEnable(GL_LIGHT4);
    glEnable(GL_LIGHT5);

    // glColor will also set the material colour of the lit objects
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);          // normals stay correct after glScalef

    // attenuation = the light gets weaker with distance
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0008f);   // meteor: weak when far
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.12f);        // campfire: only the camp
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.015f);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.008f);

    // the spot light is a cone
    glLightf(GL_LIGHT5, GL_SPOT_CUTOFF, 80.0f);
    glLightf(GL_LIGHT5, GL_QUADRATIC_ATTENUATION, 0.006f);
}

// We make a small 64 x 64 "grass" picture ourselves (no image file needed).
void setupTextures()
{
    unsigned char pixels[64 * 64 * 3];
    for (int j = 0; j < 64; j++)
    {
        for (int i = 0; i < 64; i++)
        {
            int n = (i * 7 + j * 13 + ((i * j) % 11) * 5) % 40;   // a simple "random" pattern
            pixels[(j * 64 + i) * 3 + 0] = 60 + n;                // red
            pixels[(j * 64 + i) * 3 + 1] = 130 + n * 2;           // green
            pixels[(j * 64 + i) * 3 + 2] = 55 + n / 2;            // blue
        }
    }
    glGenTextures(1, &grassTexture);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void makeVBO(GLuint *id, float *data, int numFloats)
{
    glGenBuffers(1, id);
    glBindBuffer(GL_ARRAY_BUFFER, *id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numFloats, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void setupShapes()
{
    // cube: 6 faces x 4 corners (bottom at y = 0, top at y = 1)
    float cube[] = {
        -0.5f, 0, -0.5f,   0.5f, 0, -0.5f,   0.5f, 1, -0.5f,  -0.5f, 1, -0.5f,   // back
        -0.5f, 0,  0.5f,  -0.5f, 0, -0.5f,  -0.5f, 1, -0.5f,  -0.5f, 1,  0.5f,   // left
         0.5f, 0, -0.5f,   0.5f, 0,  0.5f,   0.5f, 1,  0.5f,   0.5f, 1, -0.5f,   // right
        -0.5f, 0,  0.5f,   0.5f, 0,  0.5f,   0.5f, 1,  0.5f,  -0.5f, 1,  0.5f,   // front
        -0.5f, 1, -0.5f,   0.5f, 1, -0.5f,   0.5f, 1,  0.5f,  -0.5f, 1,  0.5f,   // top
        -0.5f, 0, -0.5f,  -0.5f, 0,  0.5f,   0.5f, 0,  0.5f,   0.5f, 0, -0.5f }; // bottom
    makeVBO(&cubeVBO, cube, 72);

    // pyramid: 4 triangles
    float pyramid[] = {
        -0.5f, 0, -0.5f,   0.5f, 0, -0.5f,   0, 1, 0,     // back
         0.5f, 0, -0.5f,   0.5f, 0,  0.5f,   0, 1, 0,     // right
         0.5f, 0,  0.5f,  -0.5f, 0,  0.5f,   0, 1, 0,     // front
        -0.5f, 0,  0.5f,  -0.5f, 0, -0.5f,   0, 1, 0 };   // left
    makeVBO(&pyramidVBO, pyramid, 36);

    // prism (the tent): 2 slanted quads, then 2 triangles for the ends
    float prism[] = {
        -0.5f, 0, -0.5f,  -0.5f, 0,  0.5f,   0, 1,  0.5f,   0, 1, -0.5f,   // left slope
         0.5f, 0,  0.5f,   0.5f, 0, -0.5f,   0, 1, -0.5f,   0, 1,  0.5f,   // right slope
        -0.5f, 0,  0.5f,   0.5f, 0,  0.5f,   0, 1,  0.5f,                  // front triangle
         0.5f, 0, -0.5f,  -0.5f, 0, -0.5f,   0, 1, -0.5f };                // back triangle
    makeVBO(&prismVBO, prism, 42);
}

void initializeStars()
{
    const float R = 190.0f;                       // the stars are on a big sphere
    for (int i = 0; i < NUM_STARS; i++)
    {
        float az   = randomRange(0.0f, 2.0f * PI);
        float sinE = randomRange(0.0f, 1.0f);     // only the upper half of the sky
        float cosE = sqrtf(1.0f - sinE * sinE);

        starPos[i * 3 + 0] = R * cosE * cosf(az);
        starPos[i * 3 + 1] = R * sinE;
        starPos[i * 3 + 2] = R * cosE * sinf(az);

        // colours: white, light blue, violet, purple
        float c = randomRange(0.0f, 1.0f);
        float r, g, b;
        if (c < 0.35f)      { r = 1.00f; g = 1.00f; b = 1.00f; }
        else if (c < 0.65f) { r = 0.75f; g = 0.87f; b = 1.00f; }
        else if (c < 0.85f) { r = 0.85f; g = 0.78f; b = 1.00f; }
        else                { r = 0.72f; g = 0.55f; b = 1.00f; }
        starTint[i * 3 + 0] = r;
        starTint[i * 3 + 1] = g;
        starTint[i * 3 + 2] = b;

        starBright[i] = randomRange(0.35f, 1.0f);
        starPhase[i]  = randomRange(0.0f, 6.28f);
    }
    updateStars();
}

// clouds all around the horizon (even numbers: low clouds, odd: high clouds)
void initializeClouds()
{
    for (int i = 0; i < NUM_CLOUDS; i++)
    {
        float az = 360.0f * i / NUM_CLOUDS + randomRange(-5.0f, 5.0f);   // degrees from north
        float elev, dist;
        if (i % 2 == 0) { elev = randomRange(5.0f, 15.0f);  dist = 95.0f;  cloudScale[i] = 1.0f; }
        else            { elev = randomRange(30.0f, 60.0f); dist = 115.0f; cloudScale[i] = 1.5f; }

        float a = az * PI / 180.0f;
        cloudX[i] = dist * sinf(a);
        cloudZ[i] = -dist * cosf(a);
        cloudY[i] = EYE_HEIGHT + dist * tanf(elev * PI / 180.0f);
        cloudAz[i] = az;
    }
}

// the trees: the 12 original trees stay, then random trees fill the forest.
// A random spot is rejected when it is
//   - inside the central clearing (|x| and |z| smaller than 12)
//   - on the entrance path (in front of the sign)
//   - too close to another tree or to a rock
void setupTrees()
{
    // the original fixed trees (they are kept exactly as they were)
    float fixedX[12] = { -16, -17, -15, -17, -14, 15, 17, 16, 14, 17, -8, 9 };
    float fixedZ[12] = { -10,  -4,   3,  10,  15, -12, -5,  2,  9, 14, 17, 17 };
    float fixedS[12] = { 1.0f, 1.2f, 0.9f, 1.3f, 1.0f, 1.1f, 0.8f, 1.2f, 1.0f, 0.9f, 1.1f, 1.3f };
    for (int i = 0; i < 12; i++)
    {
        treeX[treeCount] = fixedX[i];
        treeZ[treeCount] = fixedZ[i];
        treeScale[treeCount] = fixedS[i];
        treeCount++;
    }

    // the rocks of drawCampsite() (only used so no tree grows inside a rock)
    float rockX[6] = { -15, -15, 15, 15, -10, 11 };
    float rockZ[6] = {  -8,   4, -9,  2, -18, -18 };

    srand(treeSeed);

    int attempts = 0;
    while (treeCount < MAX_TREES && attempts < 5000)
    {
        attempts++;

        // a possible spot on the ground
        float x = randomRange(-TREE_AREA, TREE_AREA);
        float z = randomRange(-TREE_AREA, TREE_AREA);

        // inside the clearing: try again
        if (fabsf(x) < 12.0f && fabsf(z) < 12.0f)
            continue;

        // entrance path: try again
        if (fabsf(x) < 4.5f && z > 12.0f)
            continue;

        // too close to a tree that is already placed?
        bool tooClose = false;
        for (int i = 0; i < treeCount; i++)
        {
            float dx = x - treeX[i];
            float dz = z - treeZ[i];
            if (dx * dx + dz * dz < MIN_TREE_DIST * MIN_TREE_DIST)
                tooClose = true;
        }

        // too close to a rock?
        for (int i = 0; i < 6; i++)
        {
            float dx = x - rockX[i];
            float dz = z - rockZ[i];
            if (dx * dx + dz * dz < 2.5f * 2.5f)
                tooClose = true;
        }

        if (tooClose)
            continue;

        // accepted: keep the spot and choose a size
        treeX[treeCount] = x;
        treeZ[treeCount] = z;
        treeScale[treeCount] = randomRange(0.8f, 1.3f);
        treeCount++;
    }
}

// ============================================================================
//  UPDATE FUNCTIONS  (called by the timer, they never look at the camera)
// ============================================================================

// The global clock.
//   sceneTime runs from 0 to 20 and then starts again.
//   At T_RESET (18.4 s) the screen is completely white, so we can restart the
//   sky (meteors, stars, clouds) without anyone seeing it. That is what skyTime
//   is for: skyTime = sceneTime, but after 18.4 s it starts again from 0.
//   The white flash only uses sceneTime, so it can fade out smoothly.
void updateGlobalSceneTime(float dt)
{
    if (!paused)
    {
        sceneTime += dt;
        if (sceneTime >= DURATION)
            sceneTime -= T_RESET;      // restart (same sky as skyTime after T_RESET)
    }

    if (sceneTime >= T_RESET) skyTime = sceneTime - T_RESET;
    else                      skyTime = sceneTime;

    // flash: grows from the landing (14.5 s) to full white (17.5 s), then fades out
    flash = smoothstep(T_HIT, T_WHITE, sceneTime) * (1.0f - smoothstep(T_RESET, DURATION, sceneTime));
}

// stars twinkle: their brightness goes up and down with a sine wave
void updateStars()
{
    for (int i = 0; i < NUM_STARS; i++)
    {
        float b = starBright[i] * (0.65f + 0.35f * sinf(skyTime * 1.5f + starPhase[i]));
        for (int k = 0; k < 3; k++)
        {
            float c = starTint[i * 3 + k] * b;
            starCol[i * 3 + k] = c + (1.0f - c) * flash;      // white during the flash
        }
    }
}

// ---- Meteors light the whole camp ----
// This uses only positions and times, never the camera direction, so the camp is
// lit by the meteors even when you are looking the other way.
// (Light positions must be set after gluLookAt.)
void setLight(GLenum light, float r, float g, float b)
{
    GLfloat c[] = { r, g, b, 1.0f };
    glLightfv(light, GL_DIFFUSE, c);
}
void setLightPos(GLenum light, float x, float y, float z, float w)
{
    GLfloat p[] = { x, y, z, w };      // w = 1: point light, w = 0: directional light
    glLightfv(light, GL_POSITION, p);
}

void updateMeteorLighting()
{
    float t = skyTime;

    // 1) main meteor: blue-white light that follows the head
    float head[3];
    arcPoint(mainTheta(t), head);
    float visible = smoothstep(2.0f, 12.0f, head[1]);        // dark while it is on the horizon
    setLightPos(GL_LIGHT0, head[0], head[1], head[2], 1.0f);
    setLight(GL_LIGHT0, 0.5f * visible, 0.8f * visible, 1.4f * visible);

    // 2) campfire: orange, it flickers
    float flicker = 1.2f + 0.2f * sinf(fireTime * 9.0f) + 0.12f * sinf(fireTime * 23.0f);
    setLightPos(GL_LIGHT1, 0.0f, 1.0f, -5.0f, 1.0f);
    setLight(GL_LIGHT1, 1.0f * flicker, 0.55f * flicker, 0.22f * flicker);

    // 3) small meteor: pink at the start, white when it lands
    float p[3];
    derailedPos(t, p);
    float w = smoothstep(T_SEP, T_HIT, t);                   // 0 .. 1 while it approaches
    float gone = 1.0f - smoothstep(T_HIT, T_HIT + 0.8f, t);  // it disappears in the explosion
    float k2 = (0.25f + 0.8f * w) * gone;
    setLightPos(GL_LIGHT2, p[0], p[1], p[2], 1.0f);
    setLight(GL_LIGHT2, 1.0f * k2, (0.55f + 0.45f * w) * k2, (0.7f + 0.3f * w) * k2);

    // 4) night sky: a faint violet-blue light from above (directional, w = 0)
    setLightPos(GL_LIGHT3, 0.35f, 0.85f, 0.45f, 0.0f);
    setLight(GL_LIGHT3, 0.16f, 0.14f, 0.34f);

    // 5) explosion light: starts AT THE LANDING POINT and reaches farther and
    //    farther (the attenuation gets smaller as the radius R grows)
    float e    = smoothstep(T_HIT, T_WHITE, sceneTime);
    float rise = smoothstep(T_HIT, T_HIT + 0.8f, sceneTime);
    float on   = 0.0f;
    if (sceneTime < T_RESET) on = 1.0f;
    float I = 3.0f * rise * on;
    float R = 3.0f + 70.0f * e;
    setLightPos(GL_LIGHT4, LANDING_X, LANDING_Y + 1.2f, LANDING_Z, 1.0f);
    setLight(GL_LIGHT4, 1.0f * I, 0.85f * I, 0.7f * I);
    glLightf(GL_LIGHT4, GL_QUADRATIC_ATTENUATION, 4.0f / (R * R));

    // 6) spot light: above the landing point, pointing at the FRONT of the tent.
    //    It gets stronger and whiter while the small meteor approaches.
    float ta = TENT_ANGLE * PI / 180.0f;
    float doorX = TENT_X + sinf(ta) * 2.3f;                  // the door of the tent
    float doorZ = TENT_Z + cosf(ta) * 2.3f;
    setLightPos(GL_LIGHT5, LANDING_X + 2.0f, 9.0f, LANDING_Z + 2.0f, 1.0f);
    GLfloat dir[] = { doorX - (LANDING_X + 2.0f), 0.9f - 9.0f, doorZ - (LANDING_Z + 2.0f) };
    glLightfv(GL_LIGHT5, GL_SPOT_DIRECTION, dir);
    float k5 = 2.2f * w * gone;
    setLight(GL_LIGHT5, (0.55f + 0.35f * w) * k5, (0.35f + 0.55f * w) * k5, 1.0f * k5);

    // dark navy blue ambient light. During the flash it becomes white.
    GLfloat ambient[] = { 0.12f + 0.9f * flash, 0.15f + 0.9f * flash, 0.30f + 0.9f * flash, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
}

// ============================================================================
//  DRAWING THE SKY  (depth test OFF: the sky is simply painted first)
// ============================================================================

// The sky: a ring of quads around the player with a colour gradient
// (dark -> magenta-purple -> cyan -> blue -> navy) and a cone on top.
void drawNightSky()
{
    float bandY[5] = { -60.0f, 0.0f, 22.0f, 60.0f, 140.0f };
    float bandColor[5][3] = {
        { 0.02f, 0.03f, 0.07f },     // below the horizon: almost black
        { 0.22f, 0.10f, 0.30f },     // magenta-purple glow at the horizon
        { 0.08f, 0.30f, 0.46f },     // cyan
        { 0.06f, 0.14f, 0.36f },     // blue
        { 0.03f, 0.05f, 0.16f } };   // navy
    const int SEGMENTS = 36;
    const float R = 200.0f;

    glPushMatrix();
    glTranslatef(camX, EYE_HEIGHT, camZ);        // the sky always follows the player

    for (int b = 0; b < 4; b++)
    {
        glBegin(GL_QUADS);
        for (int i = 0; i < SEGMENTS; i++)
        {
            float a1 = 2.0f * PI * i / SEGMENTS;
            float a2 = 2.0f * PI * (i + 1) / SEGMENTS;
            setColor(bandColor[b][0], bandColor[b][1], bandColor[b][2]);
            glVertex3f(R * cosf(a1), bandY[b], R * sinf(a1));
            glVertex3f(R * cosf(a2), bandY[b], R * sinf(a2));
            setColor(bandColor[b + 1][0], bandColor[b + 1][1], bandColor[b + 1][2]);
            glVertex3f(R * cosf(a2), bandY[b + 1], R * sinf(a2));
            glVertex3f(R * cosf(a1), bandY[b + 1], R * sinf(a1));
        }
        glEnd();
    }

    // the top of the sky: a fan of triangles
    glBegin(GL_TRIANGLE_FAN);
    setColor(0.02f, 0.03f, 0.10f);
    glVertex3f(0.0f, 400.0f, 0.0f);
    setColor(bandColor[4][0], bandColor[4][1], bandColor[4][2]);
    for (int i = 0; i <= SEGMENTS; i++)
    {
        float a = 2.0f * PI * i / SEGMENTS;
        glVertex3f(R * cosf(a), bandY[4], R * sinf(a));
    }
    glEnd();

    glPopMatrix();
}

// Stars: two draw calls with vertex arrays (small stars, then big stars)
void drawStars()
{
    glPushMatrix();
    glTranslatef(camX, EYE_HEIGHT, camZ);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, starPos);
    glColorPointer(3, GL_FLOAT, 0, starCol);

    glPointSize(1.5f);
    glDrawArrays(GL_POINTS, 0, NUM_SMALL);
    glPointSize(3.0f);
    glDrawArrays(GL_POINTS, NUM_SMALL, NUM_BIG);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glPopMatrix();
}

// Clouds: each cloud is 5 flat circles. They face the player. They are LIT, so
// the meteors, the campfire and the sky light colour them.
void drawClouds()
{
    float puffX[5] = { 0.0f, -9.0f, 9.0f, -4.5f, 4.5f };
    float puffY[5] = { 0.0f, -1.0f, -1.0f, 2.5f, 2.5f };
    float puffR[5] = { 6.0f, 4.5f, 4.5f, 4.0f, 4.0f };

    glEnable(GL_LIGHTING);
    for (int i = 0; i < NUM_CLOUDS; i++)
    {
        float drift = 3.0f * sinf(skyTime * 0.1f + i);      // slow movement
        glPushMatrix();
        glTranslatef(cloudX[i] + drift, cloudY[i], cloudZ[i]);
        glRotatef(-cloudAz[i], 0.0f, 1.0f, 0.0f);            // turn the cloud toward the player
        glScalef(cloudScale[i], cloudScale[i], 1.0f);
        glNormal3f(0.0f, 0.0f, 1.0f);
        setColor(0.55f, 0.65f, 0.85f);
        for (int k = 0; k < 5; k++)
            drawDisc(puffX[k], puffY[k], puffR[k]);
        glPopMatrix();
    }
    glDisable(GL_LIGHTING);
}

// Main meteor: its tail is a line of balls along the half circle.
// Five layers, the widest first: violet, magenta, blue, cyan, white core.
// The glow head is a few balls of different sizes.
void drawMainMeteor()
{
    float theta = mainTheta(skyTime);
    float layerSize[5] = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f };
    float layerColor[5][3] = {
        { 0.45f, 0.25f, 0.95f },     // violet
        { 0.95f, 0.25f, 0.75f },     // magenta
        { 0.15f, 0.40f, 1.00f },     // blue
        { 0.15f, 0.90f, 1.00f },     // cyan
        { 0.90f, 0.97f, 1.00f } };   // white core

    for (int layer = 0; layer < 5; layer++)
    {
        setColor(layerColor[layer][0], layerColor[layer][1], layerColor[layer][2]);
        for (int i = 0; i < TAIL_STEPS; i++)
        {
            float p[3];
            arcPoint(theta - i * TAIL_STEP_DEG, p);          // going backward on the arc
            if (p[1] < 4.0f) continue;                       // below the horizon: not drawn
            float radius = layerSize[layer] * (0.7f + 3.0f * i / TAIL_STEPS);   // wider far away
            drawBall(p[0], p[1], p[2], radius);
        }
    }

    // the head pulses a little
    float head[3];
    arcPoint(theta, head);
    if (head[1] > 4.0f)
    {
        float pulse = 1.0f + 0.1f * sinf(skyTime * 6.0f);
        setColor(0.20f, 0.40f, 1.00f);  drawBall(head[0], head[1], head[2], 3.0f * pulse);
        setColor(0.30f, 0.80f, 1.00f);  drawBall(head[0], head[1], head[2], 1.7f * pulse);
        setColor(1.00f, 1.00f, 1.00f);  drawBall(head[0], head[1], head[2], 0.8f * pulse);
    }
}

// Small meteor: its trail is the path it flew a moment ago (derailedPos with an
// earlier time), so the trail follows every curve. The head grows and turns
// white while it comes closer to the landing point.
void drawDerailedMeteor()
{
    float t = skyTime;
    if (t > T_HIT + 0.8f) return;                    // it is gone in the explosion

    float w = smoothstep(T_SEP, T_HIT, t);
    float size = 1.0f + 8.0f * w * w;                // bigger and bigger

    // trail: two layers (pink-white outside, white inside), far part first
    for (int layer = 0; layer < 2; layer++)
    {
        if (layer == 0) setColor(1.0f - 0.2f * w, 0.5f + 0.4f * w, 0.75f + 0.2f * w);
        else            setColor(0.9f, 0.95f, 1.0f);
        for (int k = 20; k >= 0; k--)
        {
            float p[3];
            derailedPos(t - k * 0.12f, p);
            if (p[1] < -0.2f) continue;
            float radius = (layer == 0 ? 0.55f : 0.28f) * (1.0f - k / 21.0f) * (1.0f + 3.0f * w * w);
            drawBall(p[0], p[1], p[2], radius);
        }
    }

    // head
    float p[3];
    derailedPos(t, p);
    setColor(1.0f, 0.55f + 0.45f * w, 0.75f + 0.25f * w);
    drawBall(p[0], p[1], p[2], 1.2f * size);
    setColor(1.0f, 1.0f, 1.0f);
    drawBall(p[0], p[1], p[2], 0.5f * size);
}

// Fast pale meteors: a short line with a bright point. Each one flies for about a
// second and rests for the rest of its cycle. They are about ten times faster
// than the main meteor.
void drawAdditionalMeteors()
{
    glLineWidth(2.5f);
    glPointSize(4.0f);
    for (int i = 0; i < NUM_EXTRA; i++)
    {
        float u = fmodf(skyTime / exPeriod[i] + exPhase[i], 1.0f) / 0.35f;   // 0..1 while flying
        if (u > 1.0f) continue;                                               // resting

        float dx = exEnd[i][0] - exStart[i][0];
        float dy = exEnd[i][1] - exStart[i][1];
        float dz = exEnd[i][2] - exStart[i][2];
        float length = sqrtf(dx * dx + dy * dy + dz * dz);
        float px = exStart[i][0] + dx * u;
        float py = exStart[i][1] + dy * u;
        float pz = exStart[i][2] + dz * u;
        float tail = 14.0f * sinf(PI * u);          // the tail grows and shrinks (no popping)

        float cr = exColor[i][0], cg = exColor[i][1], cb = exColor[i][2];
        glBegin(GL_LINES);
        setColor(cr * 0.15f, cg * 0.15f, cb * 0.2f);              // dark end of the tail
        glVertex3f(px - dx / length * tail, py - dy / length * tail, pz - dz / length * tail);
        setColor(cr, cg, cb);                                     // bright head
        glVertex3f(px, py, pz);
        glEnd();

        glBegin(GL_POINTS);
        setColor(1.0f, 1.0f, 1.0f);
        glVertex3f(px, py, pz);
        glEnd();
    }
}

// Explosion (drawn after the campsite, with the depth test ON): a white ball
// that grows from the landing point and a ring that spreads on the ground.
void drawExplosion()
{
    if (sceneTime >= T_RESET) return;
    float e = smoothstep(T_HIT, T_WHITE, sceneTime);
    if (e <= 0.0f) return;

    setColor(1.0f, 0.92f, 0.80f);
    drawBall(LANDING_X, LANDING_Y + 0.4f, LANDING_Z, 0.4f + 70.0f * e * e);

    // the ring: a quad strip around the landing point
    float ringRadius = 1.0f + 38.0f * e;
    float ringWidth  = 0.8f + 2.5f * e;
    float inner = ringRadius - ringWidth;
    if (inner < 0.05f) inner = 0.05f;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 40; i++)
    {
        float a = 2.0f * PI * i / 40.0f;
        glVertex3f(LANDING_X + cosf(a) * (ringRadius + ringWidth), 0.06f, LANDING_Z + sinf(a) * (ringRadius + ringWidth));
        glVertex3f(LANDING_X + cosf(a) * inner,                    0.06f, LANDING_Z + sinf(a) * inner);
    }
    glEnd();
}

// ============================================================================
//  THE CAMPSITE  (lighting ON, depth test ON)
// ============================================================================

// Every shape: move it (glTranslatef), size it (glScalef), draw the VBO face by
// face. glNormal3f tells the lighting which way each face looks.
void drawCube(float x, float y, float z, float sx, float sy, float sz, int r, int g, int b)
{
    setColor255(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);                 // x, y, z is the bottom centre
    glScalef(sx, sy, sz);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    for (int f = 0; f < 6; f++)
    {
        glNormal3fv(cubeNormals[f]);
        glDrawArrays(GL_QUADS, f * 4, 4);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopMatrix();
}

void drawPyramid(float x, float y, float z, float sx, float sy, float sz, int r, int g, int b)
{
    setColor255(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    for (int f = 0; f < 4; f++)
    {
        glNormal3fv(pyramidNormals[f]);
        glDrawArrays(GL_TRIANGLES, f * 3, 3);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopMatrix();
}

void drawPrism(float x, float y, float z, float sx, float sy, float sz, int r, int g, int b)
{
    setColor255(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);

    glBindBuffer(GL_ARRAY_BUFFER, prismVBO);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glNormal3f(-1.0f, 0.5f, 0.0f);  glDrawArrays(GL_QUADS, 0, 4);        // left slope
    glNormal3f( 1.0f, 0.5f, 0.0f);  glDrawArrays(GL_QUADS, 4, 4);        // right slope
    glNormal3f( 0.0f, 0.0f, 1.0f);  glDrawArrays(GL_TRIANGLES, 8, 3);    // front
    glNormal3f( 0.0f, 0.0f, -1.0f); glDrawArrays(GL_TRIANGLES, 11, 3);   // back
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopMatrix();
}

// The ground: 2 m x 2 m tiles with the grass texture (texture coordinates 0..1).
// Small tiles are needed because the lighting is calculated at the corners.
void drawGround()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glNormal3f(0.0f, 1.0f, 0.0f);

    for (int ix = -GROUND_HALF; ix < GROUND_HALF; ix += 2)
    {
        for (int iz = -GROUND_HALF; iz < GROUND_HALF; iz += 2)
        {
            // lighter green inside the clearing (|x| and |z| smaller than 12)
            if (ix >= -12 && ix < 12 && iz >= -12 && iz < 12) setColor(1.0f, 1.0f, 1.0f);
            else                                              setColor(0.65f, 0.8f, 0.6f);

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f((float)ix,       0.0f, (float)iz);
            glTexCoord2f(1.0f, 0.0f); glVertex3f((float)ix + 2.0f, 0.0f, (float)iz);
            glTexCoord2f(1.0f, 1.0f); glVertex3f((float)ix + 2.0f, 0.0f, (float)iz + 2.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f((float)ix,       0.0f, (float)iz + 2.0f);
            glEnd();
        }
    }
    glDisable(GL_TEXTURE_2D);
}

// a flame piece: it grows and shrinks, and it glows (emission, Module 8)
void drawFlame(float x, float y, float z, float size, float speed, float phase, int r, int g, int b)
{
    float s = 0.65f + 0.35f * sinf(fireTime * speed + phase);
    GLfloat glow[] = { r / 255.0f * 0.9f, g / 255.0f * 0.9f, b / 255.0f * 0.9f, 1.0f };
    GLfloat none[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, glow);
    drawPyramid(x, y, z, size * s * 0.7f, size * s, size * s * 0.7f, r, g, b);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, none);
}

void drawCampfire(float x, float z)
{
    drawCube(x + 0.8f, 0.0f, z, 0.4f, 0.3f, 0.4f, 120, 120, 125);     // 4 stones
    drawCube(x - 0.8f, 0.0f, z, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x, 0.0f, z + 0.8f, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x, 0.0f, z - 0.8f, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x, 0.0f, z, 1.2f, 0.25f, 0.25f, 90, 55, 30);             // log

    drawFlame(x, 0.25f, z, 1.0f, 6.0f, 0.0f, 255, 120, 30);
    drawFlame(x + 0.25f, 0.25f, z, 0.6f, 8.0f, 2.0f, 230, 70, 20);
    drawFlame(x - 0.2f, 0.25f, z, 0.5f, 10.0f, 4.0f, 255, 210, 60);
}

// A tent: glRotatef turns it so that the door faces the campfire.
void drawTent(float x, float z, float angle, int r, int g, int b)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(angle, 0.0f, 1.0f, 0.0f);
    drawPrism(0.0f, 0.0f, 0.0f, 3.6f, 2.6f, 4.6f, r, g, b);               // the tent
    drawPrism(0.0f, 0.0f, 2.32f, 1.6f, 1.7f, 0.05f, 185, 140, 80);        // the door
    glPopMatrix();
}

void drawPineTree(float x, float z, float s)
{
    drawCube(x, 0.0f, z, 0.4f * s, 1.3f * s, 0.4f * s, 85, 55, 30);               // trunk
    drawPyramid(x, 1.0f * s, z, 2.6f * s, 2.0f * s, 2.6f * s, 30, 90, 50);        // lower leaves
    drawPyramid(x, 2.4f * s, z, 2.0f * s, 1.8f * s, 2.0f * s, 36, 105, 58);       // upper leaves
}

// draws all the trees from setupTrees()
void drawTrees()
{
    for (int i = 0; i < treeCount; i++)
        drawPineTree(treeX[i], treeZ[i], treeScale[i]);
}

void drawLantern(float x, float z)
{
    drawCube(x, 0.0f, z, 0.1f, 1.6f, 0.1f, 70, 45, 25);                  // post
    GLfloat glow[] = { 0.9f, 0.7f, 0.3f, 1.0f };
    GLfloat none[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, glow);                  // the lamp glows
    drawCube(x, 1.6f, z, 0.3f, 0.4f, 0.3f, 255, 215, 100);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, none);
}

void drawCampsite()
{
    drawGround();

    // cliff: the ground ends at z = -GROUND_HALF (-50)
    // edge strip: as wide as the ground, right behind the last row of tiles
    drawCube(0.0f, -0.2f, -GROUND_HALF - 0.2f, 2.0f * GROUND_HALF, 0.2f, 0.4f, 110, 95, 70);
    // the far ground below the cliff (same depth 26 and twice the ground width, as before)
    drawCube(0.0f, -26.0f, -GROUND_HALF - 13.0f, 4.0f * GROUND_HALF, 1.0f, 26.0f, 28, 60, 45);

    drawCampfire(0.0f, -5.0f);

    drawTent(-8.0f, -3.0f, 104.0f, 253, 208, 120);      // the doors face the campfire
    drawTent( 8.0f, -3.0f, -104.0f, 245, 185, 100);

    // entrance sign
    drawCube(-1.8f, 0.0f, 15.0f, 0.25f, 2.6f, 0.25f, 95, 62, 35);
    drawCube( 1.8f, 0.0f, 15.0f, 0.25f, 2.6f, 0.25f, 95, 62, 35);
    drawCube( 0.0f, 1.5f, 15.0f, 3.8f, 0.9f, 0.15f, 140, 95, 55);

    // lanterns
    drawLantern(-4.5f, -3.0f);
    drawLantern( 4.5f, -3.0f);
    drawLantern(-2.0f, -6.0f);
    drawLantern( 2.0f, -6.0f);
    drawLantern(-3.0f, 15.0f);
    drawLantern( 3.0f, 15.0f);

    // trees (the 12 original ones + the random ones, see setupTrees)
    drawTrees();

    // rocks
    drawPyramid(-15.0f, 0.0f,  -8.0f, 1.4f, 1.0f, 1.2f, 110, 110, 115);
    drawPyramid(-15.0f, 0.0f,   4.0f, 1.8f, 1.3f, 1.6f, 110, 110, 115);
    drawPyramid( 15.0f, 0.0f,  -9.0f, 1.5f, 1.1f, 1.3f, 110, 110, 115);
    drawPyramid( 15.0f, 0.0f,   2.0f, 2.0f, 1.4f, 1.7f, 110, 110, 115);
    drawPyramid(-10.0f, 0.0f, -18.0f, 1.7f, 1.2f, 1.4f, 110, 110, 115);
    drawPyramid( 11.0f, 0.0f, -18.0f, 1.4f, 1.0f, 1.2f, 110, 110, 115);
}

// ============================================================================
//  HUD: text on the screen (Module 3: glRasterPos2f + glutBitmapString)
//  We switch to a 2D projection (glOrtho2D, Module 7) just for the text.
// ============================================================================
void drawHUD()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    char text[120];
    if (paused) sprintf(text, "Time %.1f / %.0f s   (paused)", sceneTime, DURATION);
    else        sprintf(text, "Time %.1f / %.0f s", sceneTime, DURATION);
    glColor3f(1.0f, 1.0f, 0.4f);
    glRasterPos2f(10.0f, (float)winH - 22.0f);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char *)text);

    glColor3f(0.8f, 0.9f, 1.0f);
    glRasterPos2f(10.0f, (float)winH - 44.0f);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12,
        (const unsigned char *)"WASD walk | mouse look | [ ] or click = timeline | SPACE pause | R restart | C camera | F fullscreen | ESC quit");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
}

// ============================================================================
//  display: the drawing order
//    1. sky pass     (depth test off):  sky, stars, clouds, meteors
//    2. campsite     (depth test on, lighting on)
//    3. explosion    (depth test on)
//    4. text
//  The sky is painted first, so the campsite is always drawn in front of it.
// ============================================================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // camera (Module 7: gluLookAt)
    float yr = yaw * PI / 180.0f;
    float pr = pitch * PI / 180.0f;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camX, EYE_HEIGHT, camZ,
              camX + cosf(pr) * cosf(yr), EYE_HEIGHT + sinf(pr), camZ + cosf(pr) * sinf(yr),
              0.0f, 1.0f, 0.0f);

    updateMeteorLighting();

    // 1. the sky
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    drawNightSky();
    drawStars();
    drawClouds();
    drawAdditionalMeteors();
    drawMainMeteor();
    drawDerailedMeteor();

    // 2. the campsite
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    drawCampsite();

    // 3. the explosion
    glDisable(GL_LIGHTING);
    drawExplosion();

    // 4. the text
    drawHUD();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboardPress(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'w': wPressed = true; break;
        case 's': sPressed = true; break;
        case 'a': aPressed = true; break;
        case 'd': dPressed = true; break;
        
        case ' ':  paused = !paused;  break;                  // pause / resume the clock
        case 'r':  sceneTime = 0.0f;  break;                  // restart the timeline
        case 'c':  camX = 0.0f; camZ = 0.0f; yaw = -90.0f; pitch = 0.0f;  break;

        case '[':  sceneTime -= 1.0f;  if (sceneTime < 0.0f) sceneTime = 0.0f;  break;
        case ']':  sceneTime += 1.0f;  if (sceneTime > DURATION) sceneTime = DURATION;  break;

        case 'f':
            if (!fullscreen) glutFullScreen();
            else { glutReshapeWindow(1024, 768); glutPositionWindow(100, 50); }
            fullscreen = !fullscreen;
            break; 

        case 27:   exit(0);   
    }
}

void keyboardRelease(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'w': wPressed = false; break;
        case 's': sPressed = false; break;
        case 'a': aPressed = false; break;
        case 'd': dPressed = false; break;
    }
}

void movementLogic()
{
    float yr = yaw * PI / 180.0f;
    float step = 0.0000025f;

    if (wPressed)
    {
        camX += cosf(yr) * step;  camZ += sinf(yr) * step;
    }

    if (aPressed)
    {
        camX += sinf(yr) * step;  camZ -= cosf(yr) * step;
    }

    if (sPressed)
    {
        camX -= cosf(yr) * step;  camZ -= sinf(yr) * step;
    }

    if (dPressed)
    {
        camX -= sinf(yr) * step;  camZ += cosf(yr) * step; 
    }

    if (camX < minX)
        camX = minX;

    if (camX > maxX)
        camX = maxX;

    if (camZ < minZ)
        camZ = minZ;

    if (camZ > maxZ)
        camZ = maxZ;
}

// left click = forward in time, right click = back, mouse wheel = forward / back
// (many systems report the wheel as buttons 3 and 4)
void mouse(int button, int state, int x, int y)
{
    if (state != GLUT_DOWN) return;
    if (button == GLUT_LEFT_BUTTON || button == 3) sceneTime += 1.0f;
    if (button == GLUT_RIGHT_BUTTON || button == 4) sceneTime -= 1.0f;
    if (sceneTime < 0.0f) sceneTime = 0.0f;
    if (sceneTime > DURATION) sceneTime = DURATION;
}

// The mouse position decides where we look:
//   left edge to right edge of the window = one full turn (360 degrees)
//   top of the window = look straight up, bottom = look down
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

void timer(int value)
{
    int now = glutGet(GLUT_ELAPSED_TIME);        // milliseconds since the program started
    float dt = (now - lastMs) / 1000.0f;         // seconds since the last frame
    lastMs = now;
    if (dt > 0.1f) dt = 0.1f;

    fireTime = now / 1000.0f;
    updateGlobalSceneTime(dt);
    updateStars();

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(1024, 768);
    glutInitWindowPosition(100, 50);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow("Night Campsite with a Meteor Sky");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardPress);
    glutKeyboardUpFunc(keyboardRelease);
    glutIdleFunc(movementLogic);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mouseMovement);

    GLenum err = glewInit();                     // needed for the VBO functions (Module 5)
    if (err == GLEW_OK)
    {
        initializeOpenGL();
        glutFullScreen();                        // open in fullscreen
        fullscreen = true;
        lastMs = glutGet(GLUT_ELAPSED_TIME);
        glutTimerFunc(16, timer, 0);
        glutMainLoop();
    }
    return 0;
}