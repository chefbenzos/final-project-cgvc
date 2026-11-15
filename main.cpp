// ============================================================================
//  NIGHT CAMPSITE WITH A METEOR SKY            (combined program, version 4)
//  Introductory OpenGL course project
//
//  Combines:   (A) the 3D campsite free-look scene   (tents, campfire, trees ...)
//              (B) the animated meteor sky           (comet, derailed meteor,
//                  fast meteors, stars, clouds, explosion, timeline)
//
//  Uses ONLY: C++, GLUT/FreeGLUT and the traditional fixed-function pipeline
//  (primitives, vertex arrays, VBOs, the matrix stack, fixed-function lighting,
//  fog, alpha blending and textures generated in code).
//  NO shaders, NO GLSL, NO GLFW/GLAD/GLEW/SDL, NO image files.
//
//  VERSION 4 - what changed (search for [NEW] and [MODIFIED]):
//    1. The main meteor sweeps the whole sky (0 to 180 degrees) on a world-space arc. Its tail
//       reaches far into the rear sky, so it is visible from behind.
//    2. ONLY the small meteor lands, at one fixed point in front of the left tent.
//    3. The derailed meteor follows a smooth Bezier path (no spiral, no spin).
//    4. The sky is fuller: more stars, a Milky Way band, haze layers, clouds
//       all around, faint fast meteors in every direction.
//    5. The campsite is lit like a night scene (dark navy ambient, violet-blue
//       meteor light, spotlight on the tent front, explosion light).
//
//  ----------------------------------------------------------------------------
//  COORDINATE SYSTEM  (right-handed, 1 unit = 1 metre)
//    +Y is up.  The ground is the plane y = 0.
//    The campsite clearing is the square |x| <= 12, |z| <= 12 around the origin.
//    NORTH is -Z: the cliff edge is at z = -20.  The player starts at (0, 2, 0)
//    and looks north (yaw = -90 degrees).
//
//    LEFT TENT   : centre (-8, 0, -3), turned 104 degrees, its door faces +X
//                  (toward the campfire): door direction = (0.970, 0, -0.242).
//    LANDING POINT = (-4.3, 0.05, -5.8): from the door centre (-5.77, 0, -3.56)
//                  go 2.0 m straight out of the door, then 1.8 m to the far
//                  (north) side. It is in front of the tent, not inside it, and
//                  clear of the campfire (0,-5) and the lanterns.
//
//    MAIN METEOR PATH (world space): a circular arc of radius 44 m around the
//                  player in a vertical plane. The arc angle runs 0 -> 180 degrees:
//                  0 = rear horizon (azimuth 141 deg, behind-right), 90 = straight
//                  overhead, 180 = front horizon (azimuth 321 deg, over the tent
//                  side). The main meteor does NOT land: it sets over the front
//                  horizon. The tail follows the same arc for 75 degrees.
//                  The small meteor separates from it at arc angle 39.7 degrees
//                  (about 5 s), the place where the old version started.
//
//    SKY DOME : a big sphere (radius 250) that always follows the camera:
//               gradient, stars, haze layers and a far mountain ridge.
//    SKY PANELS : the fast meteors and the cloud group of the original meteor
//               scene are mounted in 1280 x 720 "panels" at distance 80,
//               uniform scale 0.0875 m per pixel (never stretched).
//               Panels exist at several compass directions.
//    BAND CLOUDS : soft cloud billboards all around the horizon (distance 95,
//               elevation 4-15 degrees) and thinner high clouds (distance 115).
//  ----------------------------------------------------------------------------
//
//  Build (Linux):    g++ main.cpp -o campsite -lglut -lGLU -lGL
//  Build (MinGW):    g++ main.cpp -o campsite.exe -lfreeglut -lglu32 -lopengl32
//  Build (macOS):    g++ main.cpp -o campsite -framework GLUT -framework OpenGL
//
//  CONTROLS (unchanged from version 3)
//    Mouse move        look around (mouse look, pointer is captured)
//    W A S D           walk forward / left / back / right
//    Arrow keys        look: UP / DOWN = look up / down, LEFT / RIGHT = turn
//    Mouse wheel       wind the meteor timeline forward (up) / back (down)
//                      (the animation keeps running while you scroll)
//    [  and  ]         timeline back / forward (fallback if the wheel is not reported)
//    SPACE             pause / resume the meteor clock
//    R                 restart the meteor timeline from 0
//    C                 put the camera back at the spawn point
//    M                 release / capture the mouse (handy in windowed mode)
//    F                 fullscreen <-> windowed
//    ESC               quit
// ============================================================================

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/freeglut_ext.h>
#endif

// ----------------------------------------------------------------------------
//  Constants
// ----------------------------------------------------------------------------
static const float PI = 3.14159265f;

// --- the sky panels (fast meteors + the cloud group) use a 1280 x 720 pixel frame ---
static const float WORLD_CX   = 640.0f;
static const float WORLD_CY   = 360.0f;
static const float PANEL_DIST = 80.0f;                 // panel plane is 80 m from the centre
static const float PANEL_PX   = 112.0f / 1280.0f;      // metres per pixel (uniform scale)
static const float PANEL_Y0   = 34.0f;                 // world height of pixel row 0
static const float CAM_Y      = 2.0f;                  // eye height

// region where the fast extra meteors fly (panel pixels). All of it is above the ridge.
static const float SKY_L = -250.0f, SKY_R = 1530.0f, SKY_B = 120.0f, SKY_T = 840.0f;

static const int NUM_STAR_SETS = 5;                    // 3 normal sets + 2 Milky Way sets

// --- [NEW] the left tent and the LANDING POINT (see the header comment) ---
static const float TENT_X = -8.0f, TENT_Z = -3.0f, TENT_ANGLE = 104.0f;
const float LANDING_X = -4.3f;
const float LANDING_Y =  0.05f;
const float LANDING_Z = -5.8f;

// --- [NEW] main meteor path (world space) ---
static const float PATH_HX = 0.62f;       // horizontal unit direction from the landing point
static const float PATH_HZ = 0.785f;      //   toward the rear sky   (0.62^2 + 0.785^2 = 1)
// [MODIFIED] the main meteor flies on a circular ARC around the camp, in the vertical plane
// whose horizontal direction is (PATH_HX, PATH_HZ). The arc angle theta runs from
// 0 degrees (rear horizon) over 90 degrees (straight overhead) to 180 degrees (front horizon).
static const float MAIN_R         = 44.0f;    // radius of the arc around the player (metres)
static const float MAIN_THETA_SEP = 39.7f;    // arc angle where the small meteor breaks away
                                              //   (the old start point, behind-right and high)
static const float T_MAIN_END     = 17.0f;    // time when the main meteor reaches 180 degrees
static const float TAIL_ARC       = 75.0f;    // tail length in degrees of arc
static const float TAIL_SCALE     = 0.04f;    // metres per "tail pixel" of the width profile
static const float MAIN_MAX_HW = 140.0f;  // widest half-width of the tail layers (tail pixels)

// --- [NEW] where the derailed meteor starts its final straight approach (front-left sky) ---
static const float APPROACH_X = -10.3f, APPROACH_Y = 14.0f, APPROACH_Z = -15.8f;

// ----------------------------------------------------------------------------
//  THE GLOBAL TIMELINE
//  sceneTime is advanced by the timer EVERY frame. It does not know where the
//  camera looks. The camera can only change what is DRAWN, never what is COMPUTED.
//
//    0.0 -  5.0  Stage 1: the small fragment is ATTACHED to the main meteor
//                (the main meteor itself sweeps 0 -> 180 degrees and never lands)
//    5.0 -  8.0  Stage 2: SMOOTH SEPARATION   (blend weight 0 -> 1, smoothstep)
//    8.0 - 11.5  Stage 3: CONTROLLED CURVE    (cubic Bezier into the front-left sky)
//   11.5 - 14.5  Stage 4: STRAIGHT APPROACH   (straight line to the landing point)
//   14.5 - 18.4  Stage 5: EXPLOSION at the landing point, light expands, screen whitens
//   18.4 - 20.0  the sky has already restarted underneath, the white fades out
// ----------------------------------------------------------------------------
float       sceneTime     = 0.0f;
const float sceneDuration = 20.0f;

static const float T_SEP           = 5.0f;     // fragment starts to leave the main meteor
static const float T_SEP_END       = 8.0f;     // separation finished (pure Bezier from here)
static const float T_CURVE_END     = 11.5f;    // curve ends, straight approach begins
static const float T_HIT           = 14.5f;    // the small meteor reaches the landing point
static const float T_FADE_IN_START = 15.5f;    // full-screen white starts to fade in
static const float T_FULL_WHITE    = 17.5f;    // the screen is completely white
static const float T_RESET         = 18.4f;    // sky restarts underneath the white
static const float WHEEL_STEP      = 1.0f;     // seconds of timeline per wheel notch

// ----------------------------------------------------------------------------
//  Tiny helpers
// ----------------------------------------------------------------------------
static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static float lerpf(float a, float b, float t)   { return a + (b - a) * t; }
static float smoothstep(float a, float b, float x)
{
    float t = clampf((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
static float deg2rad(float d) { return d * PI / 180.0f; }

// Deterministic random numbers: the same sky on every run and every computer
static unsigned int g_seed = 7u;
static float frand01()
{
    g_seed = g_seed * 1664525u + 1013904223u;
    return (float)(g_seed >> 8) / 16777216.0f;
}
static float frand(float a, float b) { return a + (b - a) * frand01(); }

static float hash2(int x, int y)
{
    unsigned n = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= (n >> 16);
    return (float)(n & 0xFFFFu) / 65535.0f;
}
static float hash3(int a, int b, int c) { return hash2(a * 9173 + c * 31, b * 7727 + c * 131 + a); }
static float valueNoise(float x, float y)
{
    int   ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - (float)ix,  fy = y - (float)iy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float a = hash2(ix, iy),     b = hash2(ix + 1, iy);
    float c = hash2(ix, iy + 1), d = hash2(ix + 1, iy + 1);
    float top = a + (b - a) * fx;
    float bot = c + (d - c) * fx;
    return top + (bot - top) * fy;
}

struct Vec2
{
    float x, y;
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float a, float b) : x(a), y(b) {}
    Vec2 operator+(const Vec2 &o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2 &o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const       { return Vec2(x * s, y * s); }
};
struct Vec3
{
    float x, y, z;
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float a, float b, float c) : x(a), y(b), z(c) {}
    Vec3 operator+(const Vec3 &o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3 &o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const       { return Vec3(x * s, y * s, z * s); }
};
static float dot3(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3  cross3(const Vec3 &a, const Vec3 &b)
{
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static Vec3  norm3(const Vec3 &v)
{
    float l = sqrtf(dot3(v, v));
    return (l > 1e-6f) ? v * (1.0f / l) : Vec3(0.0f, 1.0f, 0.0f);
}

// ----------------------------------------------------------------------------
//  VBO access (OpenGL 1.5 functions fetched through FreeGLUT: no GLEW needed)
// ----------------------------------------------------------------------------
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif

typedef ptrdiff_t VboSize;

#ifndef __APPLE__
typedef void (APIENTRY *GenBuffersFn)(GLsizei, GLuint *);
typedef void (APIENTRY *BindBufferFn)(GLenum, GLuint);
typedef void (APIENTRY *BufferDataFn)(GLenum, VboSize, const void *, GLenum);
static GenBuffersFn myGenBuffers = 0;
static BindBufferFn myBindBuffer = 0;
static BufferDataFn myBufferData = 0;
#endif

static bool g_useVBO = false;

static bool loadVBOFunctions()
{
#ifdef __APPLE__
    return true;
#else
    myGenBuffers = (GenBuffersFn)glutGetProcAddress("glGenBuffers");
    myBindBuffer = (BindBufferFn)glutGetProcAddress("glBindBuffer");
    myBufferData = (BufferDataFn)glutGetProcAddress("glBufferData");
    return myGenBuffers && myBindBuffer && myBufferData;
#endif
}
static void vboGen(GLuint *id)
{
#ifdef __APPLE__
    glGenBuffers(1, id);
#else
    myGenBuffers(1, id);
#endif
}
static void vboBind(GLuint id)
{
#ifdef __APPLE__
    glBindBuffer(GL_ARRAY_BUFFER, id);
#else
    myBindBuffer(GL_ARRAY_BUFFER, id);
#endif
}
static void vboData(VboSize bytes, const void *data)
{
#ifdef __APPLE__
    glBufferData(GL_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
#else
    myBufferData(GL_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
#endif
}

// A GpuBuffer keeps the float data AND (if possible) a VBO copy of it.
// useBuffer() returns what must be passed to glVertexPointer / glColorPointer:
// an offset 0 when the VBO is bound, or a normal array pointer as a fallback.
struct GpuBuffer
{
    GLuint id;
    std::vector<float> data;
    GpuBuffer() : id(0) {}
};
static void uploadBuffer(GpuBuffer &b)
{
    if (!g_useVBO || b.data.empty()) return;
    vboGen(&b.id);
    vboBind(b.id);
    vboData((VboSize)(b.data.size() * sizeof(float)), &b.data[0]);
    vboBind(0);
}
static const char *useBuffer(const GpuBuffer &b)
{
    if (g_useVBO) { vboBind(b.id); return (const char *)0; }
    return (const char *)&b.data[0];
}
static void releaseBuffer() { if (g_useVBO) vboBind(0); }

// [NEW] re-upload a buffer whose data changes every frame (the main meteor tail)
static void refreshBuffer(GpuBuffer &b)
{
    if (!g_useVBO || b.data.empty()) return;
    if (b.id == 0) vboGen(&b.id);
    vboBind(b.id);
    vboData((VboSize)(b.data.size() * sizeof(float)), &b.data[0]);
    vboBind(0);
}

// ----------------------------------------------------------------------------
//  Data structures
// ----------------------------------------------------------------------------
struct Vtx   { float x, y, r, g, b, a; };                  // 2D meteor vertex (position + colour)
struct Range { GLint first; GLsizei count; GLenum mode; }; // one glDrawArrays call

static Vtx makeVtx(float x, float y, float r, float g, float b, float a)
{
    Vtx v; v.x = x; v.y = y; v.r = r; v.g = g; v.b = b; v.a = a; return v;
}
static Range makeRange(GLint first, GLsizei count, GLenum mode)
{
    Range r; r.first = first; r.count = count; r.mode = mode; return r;
}

// [NEW] Stars live on the sky dome: 3D positions (x,y,z) and colours (r,g,b,a)
struct StarSet
{
    float size;
    std::vector<float> pos;      // x,y,z   (on a sphere of radius 200 around the camera)
    std::vector<float> col;      // r,g,b,a (alpha changes when twinkling)
    std::vector<float> base;
    std::vector<float> phase;
    std::vector<float> speed;
    std::vector<char>  twinkle;
};
static StarSet g_stars[NUM_STAR_SETS];

// Clouds: panel clouds (meteor-pixel units) and band clouds (world units, 3D billboards)
struct Puff  { float ox, oy, r, rot, shade; };
struct Cloud
{
    float x, y, z, az, phase, alphaScale;
    std::vector<Puff> puffs;
    Cloud() : x(0.0f), y(0.0f), z(0.0f), az(0.0f), phase(0.0f), alphaScale(1.0f) {}
};
static std::vector<Cloud> g_panelClouds;
static std::vector<Cloud> g_bandClouds;
static std::vector<Cloud> g_highClouds;     // [NEW] thin high clouds, all around

// [NEW] 3D vertex used by the main meteor tail, the derailed trail and the shock ring
struct Vtx3 { float x, y, z, r, g, b, a; };
static Vtx3 make3(const Vec3 &p, float r, float g, float b, float a)
{
    Vtx3 v; v.x = p.x; v.y = p.y; v.z = p.z; v.r = r; v.g = g; v.b = b; v.a = a; return v;
}

// [NEW] Main meteor tail: rebuilt every frame in world space, uploaded to a VBO
static std::vector<Vtx3>  g_tailVerts;
static std::vector<Range> g_tailRanges;
static GpuBuffer          g_tailBuf;

// [NEW] Derailed meteor trail: rebuilt every frame as a camera-facing ribbon
static std::vector<Vtx3>  g_derailVerts;
static std::vector<Range> g_derailRanges;

// [NEW] Soft coloured haze patches on the sky dome (translucent atmospheric layers)
struct Haze { float dx, dy, dz, size, r, g, b, a, phase; };
static std::vector<Haze> g_hazes;

struct ExtraMeteor
{
    float speed, length, halfWidth, headSize, alpha, baseAngle, period, phase;
    float r, g, b;
    std::vector<Vtx>   trail;
    std::vector<Range> ranges;
    bool  active;
    float x, y, trailAngle, fade, panelAz;
};
static std::vector<ExtraMeteor> g_extra;

// Sky dome + far ridge (one VBO, 7 floats per vertex: x,y,z,r,g,b,a)
static GpuBuffer          g_skyBuf;
static std::vector<Range> g_domeRanges;
static std::vector<Range> g_ridgeRanges;

// Textures
static GLuint g_glowTex  = 0;
static GLuint g_cloudTex = 0;

// ----------------------------------------------------------------------------
//  Campsite data (from the campsite program)
// ----------------------------------------------------------------------------
static GpuBuffer planeBuf, cubeBuf, pyramidBuf, prismBuf, groundBuf;
static int planeVertexCount = 0;
static int groundGrassCount = 0, groundClearCount = 0;

static float fireTime = 0.0f;                  // real seconds, used by the campfire

static const int MAX_TREES = 30;
static float treeX[MAX_TREES];
static float treeZ[MAX_TREES];
static float treeScale[MAX_TREES];
static int   treeCount = 0;
static int   treeSeed  = 25;                   // change this number to get a different forest

// ----------------------------------------------------------------------------
//  Camera / input state (campsite program)
// ----------------------------------------------------------------------------
static float camX = 0.0f, camY = CAM_Y, camZ = 0.0f;
static float yaw = -90.0f, pitch = 0.0f;
static float sensitivity = 0.1f;
static float frontX = 0.0f, frontY = 0.0f, frontZ = -1.0f;
static bool  wPressed = false, aPressed = false, sPressed = false, dPressed = false;
static bool  leftArrow = false, rightArrow = false, upArrow = false, downArrow = false;
static bool  firstMouse = true;
static bool  mouseLook  = true;
static const float MOVE_SPEED = 4.0f;          // metres per second
static const float LOOK_RATE  = 70.0f;         // degrees per second (arrow keys)

// ----------------------------------------------------------------------------
//  Animation / window state
// ----------------------------------------------------------------------------
static bool  g_paused       = false;
static float g_worldTime    = 0.0f;   // the time the sky objects use (see updateGlobalSceneTime)
static float g_scrubPending = 0.0f;

static Vec3  g_mainHead;              // [NEW] main meteor head in world space
static float g_mainTheta = 0.0f;      // [MODIFIED] arc angle of the main head (0 .. 180 degrees)
static float g_mainPulse = 1.0f;

static Vec3  g_derailPos;             // [NEW] derailed meteor state (world space)
static Vec3  g_derailDir  = Vec3(-0.5f, -0.7f, -0.5f);   // direction of travel (unit)
static float g_derailScale  = 1.0f;
static float g_derailBright = 0.8f;

static bool  g_fullscreen = false;
static int   g_savedX = 100, g_savedY = 100, g_savedW = 1280, g_savedH = 720;

// ----------------------------------------------------------------------------
//  Forward declarations
// ----------------------------------------------------------------------------
static void initializeOpenGL();
static void setupNightLighting();
static void setupTextures();
static void initializeStars();
static void initializeMeteors();
static void createClouds();
static void setupSkyDome();
static void setupCampsite();

static void updateGlobalSceneTime(float dt);
static void updateMainComet();
static void updateDerailedMeteor();
static void updateExtraMeteors();
static void updateStars();
static void updateMeteorAnimation();
static void updateCamera(float dt);
static void updateMeteorLighting();
static void applyMeteorGlow(float flash);

static void drawNightSky();
static void drawStars();
static void drawAtmosphere();
static void drawClouds();
static void drawPanelClouds();
static void drawMainMeteor();
static void drawDerailedMeteor();
static void drawAdditionalMeteors();
static void drawMeteorGlows();
static void drawExplosionLight();
static void drawWhiteFlash();
static void drawGround();
static void drawCampsite();

static void display();
static void reshape(int w, int h);
static void keyboard(unsigned char key, int x, int y);
static void keyboardUp(unsigned char key, int x, int y);
static void specialKeys(int key, int x, int y);
static void specialKeysUp(int key, int x, int y);
static void mouse(int button, int state, int x, int y);
static void mouseWheel(int wheel, int direction, int x, int y);
static void mouseMovement(int x, int y);
static void toggleFullscreen();
static void updateCameraDirection();

// ============================================================================
//  SMALL DRAWING HELPERS
// ============================================================================

// A square [-s, s] with texture coordinates (glows and cloud puffs). The normal
// points toward +Z, which only matters when lighting is on (clouds).
static void drawTexturedQuad(float s, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-s,  s);
    glEnd();
}

static void pushStrips(std::vector<Vtx> &verts, std::vector<Range> &ranges,
                       const std::vector<Vtx> &top, const std::vector<Vtx> &mid,
                       const std::vector<Vtx> &bot)
{
    GLint first = (GLint)verts.size();
    for (size_t i = 0; i < top.size(); ++i) { verts.push_back(top[i]); verts.push_back(mid[i]); }
    ranges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));

    first = (GLint)verts.size();
    for (size_t i = 0; i < top.size(); ++i) { verts.push_back(mid[i]); verts.push_back(bot[i]); }
    ranges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));
}

// Mount a 2D 1280x720 panel in the 3D world at compass direction azDeg
// (0 = north, 90 = east, ...): rotate about Y, move to distance 80, scale x and y
// by the SAME factor (no distortion), keep z unscaled (metres).
static void beginPanel(float azDeg)
{
    glPushMatrix();
    glRotatef(-azDeg, 0.0f, 1.0f, 0.0f);
    glTranslatef(-WORLD_CX * PANEL_PX, PANEL_Y0, -PANEL_DIST);
    glScalef(PANEL_PX, PANEL_PX, 1.0f);
}
static void endPanel() { glPopMatrix(); }

static void setEmission(float r, float g, float b)
{
    GLfloat e[] = { r, g, b, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
}

// ============================================================================
//  [NEW] 3D HELPERS
// ============================================================================

// Strips for a glowing band in 3D (the same idea as pushStrips, with 3D vertices)
static void pushStrips3(std::vector<Vtx3> &verts, std::vector<Range> &ranges,
                        const std::vector<Vtx3> &top, const std::vector<Vtx3> &mid,
                        const std::vector<Vtx3> &bot)
{
    GLint first = (GLint)verts.size();
    for (size_t i = 0; i < top.size(); ++i) { verts.push_back(top[i]); verts.push_back(mid[i]); }
    ranges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));

    first = (GLint)verts.size();
    for (size_t i = 0; i < top.size(); ++i) { verts.push_back(mid[i]); verts.push_back(bot[i]); }
    ranges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));
}

// Call right after glTranslatef(position): replaces the rotation part of the modelview
// matrix by the identity, so the sprite drawn next always faces the camera.
static void billboardRotation()
{
    float m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    m[0] = 1.0f; m[1] = 0.0f; m[2]  = 0.0f; m[3]  = 0.0f;
    m[4] = 0.0f; m[5] = 1.0f; m[6]  = 0.0f; m[7]  = 0.0f;
    m[8] = 0.0f; m[9] = 0.0f; m[10] = 1.0f; m[11] = 0.0f;
    glLoadMatrixf(m);
}

// ============================================================================
//  INITIALISATION
// ============================================================================
static void initializeOpenGL()
{
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glShadeModel(GL_SMOOTH);

    // Fog blends the far campsite objects into the dark horizon (smooth ground -> sky)
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.011f);
    glHint(GL_FOG_HINT, GL_NICEST);

    g_useVBO = loadVBOFunctions();
    if (!g_useVBO)
        std::printf("VBO functions not available - using ordinary vertex arrays instead.\n");

    g_seed = 7u;                                   // fixed seed -> same stars and clouds every run
    setupNightLighting();
    setupTextures();
    setupSkyDome();
    initializeStars();
    createClouds();
    initializeMeteors();
    setupCampsite();
    updateCameraDirection();
}

// ----------------------------------------------------------------------------
//  [MODIFIED] setupNightLighting: 8 fixed-function lights for a NIGHT scene
//    LIGHT0  main meteor head   cyan-white-blue   positional (stronger when close)
//    LIGHT1  main meteor tail   violet-magenta    positional
//    LIGHT2  derailed meteor    pink -> white     positional
//    LIGHT3  campfire           warm, flickers    positional with attenuation
//    LIGHT4  meteor shower      faint violet-blue directional
//    LIGHT5  flash              white             directional (during the flash)
//    LIGHT6  explosion          white-warm        positional AT THE LANDING POINT,
//                                                 its reach grows with time
//    LIGHT7  meteor glow        violet -> white   SPOT light on the tent front
//  The global ambient light is a dark navy blue (see applyMeteorGlow). Colours
//  and positions are updated every frame in updateMeteorLighting().
// ----------------------------------------------------------------------------
static void setupNightLighting()
{
    GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    for (int i = 0; i < 8; ++i)
    {
        GLenum L = GL_LIGHT0 + i;
        glLightfv(L, GL_AMBIENT,  black);
        glLightfv(L, GL_DIFFUSE,  black);
        glLightfv(L, GL_SPECULAR, black);
        glLightf (L, GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf (L, GL_LINEAR_ATTENUATION,    0.0f);
        glLightf (L, GL_QUADRATIC_ATTENUATION, 0.0f);
        glEnable(L);
    }
    // meteor lights fade with distance: they matter most when a meteor is close or overhead
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0008f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0006f);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.008f);
    // the campfire light only reaches the camp
    glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION,    0.12f);
    glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, 0.015f);
    // LIGHT6 (explosion): the attenuation is changed every frame so the light spreads outward
    // LIGHT7: a spot light (cone) that lights the front of the tent
    glLightf(GL_LIGHT7, GL_QUADRATIC_ATTENUATION, 0.006f);
    glLightf(GL_LIGHT7, GL_SPOT_CUTOFF,   80.0f);
    glLightf(GL_LIGHT7, GL_SPOT_EXPONENT, 1.2f);

    // dark navy blue ambient atmosphere
    GLfloat nightAmbient[] = { 0.12f, 0.15f, 0.30f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, nightAmbient);

    // glColor3f / glColor4f set the ambient + diffuse material, so every object keeps its own colour
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // specular + shininess + emission are set with glMaterialfv
    GLfloat spec[] = { 0.06f, 0.06f, 0.10f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);
    setEmission(0.0f, 0.0f, 0.0f);

    glEnable(GL_NORMALIZE);
    // GL_LIGHTING itself is switched on only for clouds and the campsite.
}

// ----------------------------------------------------------------------------
//  setupTextures: both textures are created in code (no image files)
// ----------------------------------------------------------------------------
static GLuint makeTexture(const std::vector<unsigned char> &pixels, int size)
{
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
    return id;
}

static void setupTextures()
{
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    {   // soft round glow
        const int N = 64;
        std::vector<unsigned char> px(N * N * 4);
        for (int j = 0; j < N; ++j)
            for (int i = 0; i < N; ++i)
            {
                float u = ((i + 0.5f) / N) * 2.0f - 1.0f;
                float v = ((j + 0.5f) / N) * 2.0f - 1.0f;
                float d2 = u * u + v * v;
                float a  = (d2 < 1.0f) ? expf(-5.0f * d2) * (1.0f - d2) : 0.0f;
                unsigned char *p = &px[(j * N + i) * 4];
                p[0] = p[1] = p[2] = 255;
                p[3] = (unsigned char)(clampf(a * 1.6f, 0.0f, 1.0f) * 255.0f);
            }
        g_glowTex = makeTexture(px, N);
    }

    {   // cloud puff with a noisy edge
        const int N = 128;
        std::vector<unsigned char> px(N * N * 4);
        for (int j = 0; j < N; ++j)
            for (int i = 0; i < N; ++i)
            {
                float u = ((i + 0.5f) / N) * 2.0f - 1.0f;
                float v = ((j + 0.5f) / N) * 2.0f - 1.0f;
                float d = sqrtf(u * u + v * v);
                float n = valueNoise(u * 3.0f + 5.0f, v * 3.0f + 5.0f)
                        + 0.5f * valueNoise(u * 7.0f + 11.0f, v * 7.0f + 11.0f);
                float edge = 0.70f + 0.28f * (n / 1.5f);
                float a = clampf((edge - d) / 0.30f, 0.0f, 1.0f);
                a = a * a * (3.0f - 2.0f * a);
                float shade = 0.80f + 0.20f * (v * 0.5f + 0.5f);
                unsigned char *p = &px[(j * N + i) * 4];
                p[0] = p[1] = p[2] = (unsigned char)(shade * 255.0f);
                p[3] = (unsigned char)(a * 255.0f);
            }
        g_cloudTex = makeTexture(px, N);
    }
}

// ----------------------------------------------------------------------------
//  [NEW] setupSkyDome: night-sky gradient + two layers of far mountains.
//  The gradient is stored in vertex colours (vertex array / VBO). Going up from
//  the horizon the colours are: magenta-purple -> violet -> cyan -> blue -> navy.
// ----------------------------------------------------------------------------
static void skyColor(float eDeg, float az, float &r, float &g, float &b)
{
    static const float E[9] = { -40.0f, -5.0f, 3.0f, 8.0f, 13.0f, 20.0f, 30.0f, 50.0f, 90.0f };
    static const float C[9][3] = {
        { 0.012f, 0.016f, 0.040f },     // far below the horizon: almost black
        { 0.040f, 0.040f, 0.100f },
        { 0.220f, 0.100f, 0.300f },     // magenta-purple glow right above the mountains
        { 0.200f, 0.200f, 0.440f },     // violet-blue
        { 0.080f, 0.300f, 0.460f },     // cyan band
        { 0.060f, 0.180f, 0.400f },     // blue
        { 0.060f, 0.090f, 0.280f },
        { 0.040f, 0.060f, 0.200f },
        { 0.020f, 0.030f, 0.100f }      // zenith: deep navy
    };
    float e = clampf(eDeg, E[0], E[8]);
    int i = 0;
    while (i < 7 && e > E[i + 1]) ++i;
    float t = smoothstep(E[i], E[i + 1], e);
    r = lerpf(C[i][0], C[i + 1][0], t);
    g = lerpf(C[i][1], C[i + 1][1], t);
    b = lerpf(C[i][2], C[i + 1][2], t);

    // a little more magenta on one side of the horizon
    float hz = 1.0f - smoothstep(0.0f, 22.0f, fabsf(eDeg - 6.0f));
    r += 0.05f * hz * (0.5f + 0.5f * cosf(az - 1.2f));
    b += 0.02f * hz * (0.5f + 0.5f * cosf(az - 1.2f));
}

static void pushV3(std::vector<float> &v, float x, float y, float z, float r, float g, float b, float a)
{
    v.push_back(x); v.push_back(y); v.push_back(z);
    v.push_back(r); v.push_back(g); v.push_back(b); v.push_back(a);
}

static void setupSkyDome()
{
    std::vector<float> &v = g_skyBuf.data;

    // ---- dome: rings of constant elevation, one triangle strip between neighbouring rings ----
    static const float ring[] = { -40.0f, -20.0f, -5.0f, 0.0f, 3.0f, 5.0f, 8.0f, 11.0f, 13.0f, 16.0f,
                                   20.0f, 25.0f, 30.0f, 40.0f, 50.0f, 65.0f, 80.0f, 90.0f };
    const int   nRing  = (int)(sizeof(ring) / sizeof(ring[0]));
    const int   slices = 64;
    const float R      = 250.0f;

    for (int i = 0; i + 1 < nRing; ++i)
    {
        GLint first = (GLint)(v.size() / 7);
        for (int j = 0; j <= slices; ++j)
        {
            float az = 2.0f * PI * (float)j / (float)slices;
            for (int k = 0; k < 2; ++k)
            {
                float e  = ring[i + k];
                float er = deg2rad(e);
                float r, g, b;
                skyColor(e, az, r, g, b);
                pushV3(v, R * cosf(er) * cosf(az), R * sinf(er), R * cosf(er) * sinf(az), r, g, b, 1.0f);
            }
        }
        g_domeRanges.push_back(makeRange(first, (GLsizei)(2 * (slices + 1)), GL_TRIANGLE_STRIP));
    }

    // ---- two layers of far mountains (dark silhouettes, lighter at the top = haze) ----
    for (int layer = 0; layer < 2; ++layer)
    {
        const int   n     = 120;
        const float Rr    = (layer == 0) ? 190.0f : 170.0f;
        const float baseE = (layer == 0) ? 3.0f : 0.8f;       // degrees
        const float amp   = (layer == 0) ? 3.0f : 2.2f;
        const float ph1   = 1.3f + layer * 2.1f, ph2 = 0.4f + layer * 1.7f, ph3 = 2.2f + layer * 0.9f;
        float topR = (layer == 0) ? 0.10f : 0.05f,  topG = (layer == 0) ? 0.10f : 0.055f, topB = (layer == 0) ? 0.24f : 0.12f;
        float botR = (layer == 0) ? 0.04f : 0.02f,  botG = (layer == 0) ? 0.04f : 0.025f, botB = (layer == 0) ? 0.10f : 0.06f;

        GLint first = (GLint)(v.size() / 7);
        for (int j = 0; j <= n; ++j)
        {
            float az = 2.0f * PI * (float)j / (float)n;
            float h  = baseE + amp * (0.5f * sinf(3.0f * az + ph1) + 0.3f * sinf(7.0f * az + ph2)
                                    + 0.2f * sinf(13.0f * az + ph3));
            float et = deg2rad(h), eb = deg2rad(-40.0f);
            pushV3(v, Rr * cosf(et) * cosf(az), Rr * sinf(et), Rr * cosf(et) * sinf(az), topR, topG, topB, 1.0f);
            pushV3(v, Rr * cosf(eb) * cosf(az), Rr * sinf(eb), Rr * cosf(eb) * sinf(az), botR, botG, botB, 1.0f);
        }
        g_ridgeRanges.push_back(makeRange(first, (GLsizei)(2 * (n + 1)), GL_TRIANGLE_STRIP));
    }

    uploadBuffer(g_skyBuf);
}

// ----------------------------------------------------------------------------
//  [MODIFIED] initializeStars: about 9000 stars on the whole sky dome.
//   - 3 normal sets (3 point sizes): white, light blue, violet and purple stars,
//     evenly spread over the upper hemisphere (fills the front, sides, rear, top)
//   - 2 Milky Way sets: thousands of tiny pale-blue / violet stars packed along a
//     great circle of the sky, plus soft haze patches
//  A fixed random seed gives the same sky on every run. Stars are vertex arrays.
// ----------------------------------------------------------------------------
static void addStar(StarSet &set, float dx, float dy, float dz,
                    float r, float g, float b, float bright, bool tw)
{
    const float R = 200.0f;
    set.pos.push_back(R * dx); set.pos.push_back(R * dy); set.pos.push_back(R * dz);
    set.col.push_back(r); set.col.push_back(g); set.col.push_back(b); set.col.push_back(bright);
    set.base.push_back(bright);
    set.phase.push_back(frand(0.0f, 2.0f * PI));
    set.speed.push_back(frand(0.5f, 1.8f));
    set.twinkle.push_back(tw ? 1 : 0);
}

static void initializeStars()
{
    const float sizes[NUM_STAR_SETS] = { 1.3f, 2.2f, 3.4f, 1.2f, 1.7f };
    const int   counts[3]            = { 4200, 1300, 220 };
    for (int s = 0; s < NUM_STAR_SETS; ++s) g_stars[s].size = sizes[s];

    for (int s = 0; s < 3; ++s)
        for (int i = 0; i < counts[s]; ++i)
        {
            float az   = frand(0.0f, 2.0f * PI);
            float sinE = frand(0.0f, 1.0f);                 // uniform over the upper hemisphere
            float cosE = sqrtf(1.0f - sinE * sinE);
            float b = (s == 2) ? frand(0.8f, 1.0f) : frand(0.30f, 1.0f);
            b *= 0.35f + 0.65f * smoothstep(0.0f, 0.2f, sinE);   // fainter near the horizon

            float tint = frand01();
            float r, g, bl;
            if (tint < 0.35f)      { r = 1.00f; g = 1.00f; bl = 1.00f; }   // white
            else if (tint < 0.65f) { r = 0.75f; g = 0.87f; bl = 1.00f; }   // light blue
            else if (tint < 0.85f) { r = 0.85f; g = 0.78f; bl = 1.00f; }   // faint violet
            else                   { r = 0.72f; g = 0.55f; bl = 1.00f; }   // purple

            addStar(g_stars[s], cosE * cosf(az), sinE, cosE * sinf(az), r, g, bl, b,
                    frand01() < (s == 2 ? 0.85f : 0.35f));
        }

    // ---- Milky Way: stars close to a great circle with normal n ----
    Vec3 n  = norm3(Vec3(0.30f, 0.55f, 0.78f));
    Vec3 e1 = norm3(cross3(n, Vec3(0.0f, 1.0f, 0.0f)));
    Vec3 e2 = cross3(n, e1);
    for (int s = 3; s < 5; ++s)
    {
        int want = (s == 3) ? 1900 : 650;
        int got = 0, guard = 0;
        while (got < want && guard < 20000)
        {
            ++guard;
            float phi = frand(0.0f, 2.0f * PI);
            float off = (frand01() + frand01() + frand01() - 1.5f) * 0.28f;   // roughly bell shaped
            Vec3 v = norm3(e1 * cosf(phi) + e2 * sinf(phi) + n * off);
            if (v.y < 0.02f) continue;                                       // only above the horizon
            float b = frand(0.20f, 0.70f);
            if (s == 3) addStar(g_stars[s], v.x, v.y, v.z, 0.85f, 0.92f, 1.00f, b, frand01() < 0.2f);
            else        addStar(g_stars[s], v.x, v.y, v.z, 0.78f, 0.62f, 1.00f, b * 0.9f, frand01() < 0.2f);
            ++got;
        }
    }

    // ---- soft haze patches (violet, cyan, magenta, blue) at different places and depths ----
    const float pal[4][3] = { { 0.55f, 0.30f, 1.00f }, { 0.20f, 0.80f, 1.00f },
                              { 1.00f, 0.30f, 0.80f }, { 0.30f, 0.50f, 1.00f } };
    for (int i = 0; i < 16; ++i)
    {
        float az   = frand(0.0f, 2.0f * PI);
        float elev = deg2rad(frand(12.0f, 78.0f));
        Haze h;
        h.dx = cosf(elev) * cosf(az); h.dy = sinf(elev); h.dz = cosf(elev) * sinf(az);
        h.size = frand(22.0f, 48.0f);
        h.r = pal[i % 4][0]; h.g = pal[i % 4][1]; h.b = pal[i % 4][2];
        h.a = frand(0.05f, 0.10f);
        h.phase = frand(0.0f, 6.28f);
        g_hazes.push_back(h);
    }
    for (int i = 0; i < 10; ++i)                                   // haze along the Milky Way
    {
        float phi = 2.0f * PI * (float)i / 10.0f;
        Vec3 v = norm3(e1 * cosf(phi) + e2 * sinf(phi));
        if (v.y < 0.12f) continue;
        Haze h;
        h.dx = v.x; h.dy = v.y; h.dz = v.z;
        h.size = 30.0f;
        h.r = 0.65f; h.g = 0.72f; h.b = 1.00f; h.a = 0.05f;
        h.phase = frand(0.0f, 6.28f);
        g_hazes.push_back(h);
    }
}

static void updateStars()
{
    for (int s = 0; s < NUM_STAR_SETS; ++s)
    {
        StarSet &set = g_stars[s];
        for (size_t i = 0; i < set.base.size(); ++i)
            if (set.twinkle[i])
                set.col[i * 4 + 3] = set.base[i] * (0.55f + 0.45f * sinf(g_worldTime * set.speed[i] + set.phase[i]));
    }
}

// ----------------------------------------------------------------------------
//  createClouds
//   panel clouds : the cloud groups of the meteor scene (meteor-pixel units)
//   band clouds  : [NEW] large soft clouds low in the sky between the campsite
//                  and the meteor panel (world units)
// ----------------------------------------------------------------------------
static void fillPuffs(Cloud &c, float w, float h, int count)
{
    for (int i = 0; i < count; ++i)
    {
        float ang = frand(0.0f, 2.0f * PI);
        float rad = sqrtf(frand01());
        Puff p;
        p.ox    = cosf(ang) * rad * w * 0.5f;
        p.oy    = sinf(ang) * rad * h * 0.5f - 0.08f * h;
        p.r     = h * frand(0.20f, 0.42f);
        p.rot   = frand(0.0f, 360.0f);
        p.shade = frand(0.88f, 1.10f);
        c.puffs.push_back(p);
    }
}

static void addPanelCloud(float cx, float cy, float w, float h, int count)
{
    Cloud c;
    c.x = cx; c.y = cy; c.z = 0.0f; c.az = 0.0f; c.phase = frand(0.0f, 6.28f);
    fillPuffs(c, w, h, count);
    g_panelClouds.push_back(c);
}

static void addBandCloud(std::vector<Cloud> &list, float azDeg, float elevDeg, float dist,
                         float w, float h, int count, float alphaScale)
{
    Cloud c;
    float a = deg2rad(azDeg);                         // angle from north (-Z) toward east (+X)
    c.x = dist * sinf(a);
    c.z = -dist * cosf(a);
    c.y = CAM_Y + dist * tanf(deg2rad(elevDeg));
    c.az = azDeg;
    c.phase = frand(0.0f, 6.28f);
    c.alphaScale = alphaScale;
    fillPuffs(c, w, h, count);
    list.push_back(c);
}

static void createClouds()
{
    //            x      y     width height puffs      (panel pixels)
    addPanelCloud( 720.0f, 648.0f, 230.0f,  90.0f, 14);
    addPanelCloud( 527.0f, 473.0f, 125.0f,  70.0f,  9);
    addPanelCloud(1054.0f, 535.0f, 280.0f, 200.0f, 26);
    addPanelCloud( 892.0f, 450.0f, 140.0f,  75.0f, 10);
    addPanelCloud( 828.0f, 245.0f, 250.0f, 160.0f, 22);
    addPanelCloud( 997.0f, 391.0f,  55.0f,  40.0f,  4);

    // [MODIFIED] low band clouds ALL AROUND the horizon (26 groups, about 14 degrees apart)
    for (int i = 0; i < 26; ++i)
    {
        float az   = 14.0f * (float)i + frand(-4.0f, 4.0f);
        float elev = frand(4.0f, 15.0f);
        addBandCloud(g_bandClouds, az, elev, 95.0f, frand(30.0f, 55.0f), frand(8.0f, 13.0f), 12, 1.0f);
    }
    // [NEW] thinner, larger clouds high in the sky: different depth, fills the upper sky
    for (int i = 0; i < 12; ++i)
    {
        float az   = 30.0f * (float)i + frand(-10.0f, 10.0f);
        float elev = frand(28.0f, 68.0f);
        addBandCloud(g_highClouds, az, elev, 115.0f, frand(60.0f, 100.0f), frand(14.0f, 22.0f), 14, 0.5f);
    }
}

// ----------------------------------------------------------------------------
//  initializeMeteors: fast extra meteors (the main meteor tail is rebuilt every frame)
// ----------------------------------------------------------------------------
struct LayerDef { float r, g, b, a, w, off; };



// [MODIFIED] Twelve pale, thin and fast meteors (700 - 1300 px/s; the main meteor moves
// about 7 m/s). Seven are brighter, five are faint "distant shooting stars". Each one
// belongs to a sky panel at its own compass direction, so they fly in the front,
// the sides AND behind the player.
static void initializeExtraMeteors()
{
    struct ExtraDef { float speed, len, hw, head, alpha, angle, period, phase, r, g, b, az; };
    const ExtraDef defs[] = {
        //  speed  trail  halfW head alpha angle period phase   r     g     b    panel azimuth
        {  700.0f, 170.0f, 1.6f, 6.0f, 0.50f, 215.0f, 4.6f, 0.5f, 0.70f, 0.85f, 1.00f,   0.0f },
        {  900.0f, 230.0f, 1.3f, 5.0f, 0.45f, 226.0f, 5.4f, 2.1f, 0.70f, 1.00f, 1.00f,  75.0f },
        { 1100.0f, 140.0f, 1.0f, 4.5f, 0.42f, 208.0f, 3.9f, 3.3f, 1.00f, 1.00f, 1.00f, 150.0f },
        {  800.0f, 260.0f, 1.8f, 7.0f, 0.40f, 233.0f, 6.2f, 1.2f, 0.80f, 0.70f, 1.00f, 225.0f },
        { 1250.0f, 110.0f, 0.9f, 4.0f, 0.38f, 219.0f, 4.2f, 0.0f, 0.90f, 0.95f, 1.00f, 300.0f },
        {  950.0f, 200.0f, 1.4f, 5.5f, 0.46f, 212.0f, 5.0f, 4.0f, 0.75f, 0.88f, 1.00f, 340.0f },
        { 1050.0f, 150.0f, 1.1f, 4.8f, 0.40f, 238.0f, 3.6f, 2.8f, 0.85f, 0.80f, 1.00f, 110.0f },
        // faint distant shooting stars
        { 1150.0f, 100.0f, 0.7f, 3.5f, 0.30f, 222.0f, 4.0f, 1.7f, 0.85f, 0.90f, 1.00f, 180.0f },
        {  980.0f, 130.0f, 0.8f, 3.8f, 0.28f, 210.0f, 5.6f, 3.6f, 0.80f, 0.85f, 1.00f, 255.0f },
        { 1300.0f,  90.0f, 0.6f, 3.2f, 0.26f, 230.0f, 3.8f, 0.9f, 0.90f, 0.80f, 1.00f, 330.0f },
        { 1020.0f, 120.0f, 0.7f, 3.6f, 0.30f, 217.0f, 4.8f, 2.4f, 0.75f, 0.95f, 1.00f,  45.0f },
        { 1180.0f, 105.0f, 0.6f, 3.4f, 0.27f, 236.0f, 5.2f, 4.4f, 0.95f, 0.90f, 1.00f, 120.0f }
    };

    for (size_t k = 0; k < sizeof(defs) / sizeof(defs[0]); ++k)
    {
        const ExtraDef &d = defs[k];
        ExtraMeteor m;
        m.speed = d.speed; m.length = d.len; m.halfWidth = d.hw; m.headSize = d.head;
        m.alpha = d.alpha; m.baseAngle = d.angle; m.period = d.period; m.phase = d.phase;
        m.r = d.r; m.g = d.g; m.b = d.b; m.panelAz = d.az;
        m.active = false; m.x = 0.0f; m.y = 0.0f; m.trailAngle = 0.0f; m.fade = 0.0f;

        const int N = 14;
        std::vector<Vtx> top, mid, bot;
        for (int i = 0; i <= N; ++i)
        {
            float t  = (float)i / (float)N;
            float x  = t * m.length;
            float hw = m.halfWidth * powf(1.0f - t, 0.6f) + 0.2f;
            float a  = m.alpha * powf(1.0f - t, 1.3f);
            top.push_back(makeVtx(x,  hw, m.r, m.g, m.b, 0.0f));
            mid.push_back(makeVtx(x, 0.0f, m.r, m.g, m.b, a));
            bot.push_back(makeVtx(x, -hw, m.r, m.g, m.b, 0.0f));
        }
        pushStrips(m.trail, m.ranges, top, mid, bot);
        g_extra.push_back(m);
    }
}

static void initializeMeteors()
{
    initializeExtraMeteors();
    updateMeteorAnimation();          // fills the tail buffer once, so the first frame is complete
}

// ============================================================================
//  [NEW] METEOR PATH MATH  (pure functions of the time w, all in WORLD space)
//  The meteors exist in the world whether or not the camera sees them.
// ============================================================================

static Vec3 landingPoint() { return Vec3(LANDING_X, LANDING_Y, LANDING_Z); }

// Main meteor arc: theta = 0 (rear horizon) .. 90 (overhead) .. 180 (front horizon).
// P(theta) = C + R * (cos(theta) * Hback + sin(theta) * Up),   C = the player's eye (0, 2, 0)
static Vec3 mainArcPoint(float thetaDeg)
{
    float a = deg2rad(thetaDeg);
    return Vec3(MAIN_R * cosf(a) * PATH_HX, CAM_Y + MAIN_R * sinf(a), MAIN_R * cosf(a) * PATH_HZ);
}
// unit tangent pointing BACKWARD along the arc (toward smaller theta): the tail direction
static Vec3 arcTangentBack(float thetaDeg)
{
    float a = deg2rad(thetaDeg);
    return norm3(Vec3(sinf(a) * PATH_HX, -cosf(a), sinf(a) * PATH_HZ));
}
// horizontal unit vector perpendicular to the path plane
static Vec3 pathSide() { return Vec3(PATH_HZ, 0.0f, -PATH_HX); }

// Head angle: 0 degrees at t = 0, MAIN_THETA_SEP at T_SEP (the break-away) and 180 degrees at
// T_MAIN_END, using theta = 180 * (t / T_MAIN_END)^p. The exponent p is chosen so that the
// curve passes exactly through (T_SEP, MAIN_THETA_SEP): the meteor sweeps the whole sky
// and gets a little faster while it crosses.
static float mainHeadThetaAt(float w)
{
    float u = clampf(w / T_MAIN_END, 0.0f, 1.0f);
    float p = logf(MAIN_THETA_SEP / 180.0f) / logf(T_SEP / T_MAIN_END);
    return 180.0f * powf(u, p);
}

// STAGE 1: the fragment rides next to the main head: 4 m behind it, 3 m to the side, 1.2 m lower.
// (Only the small meteor lands. At the break-away, 5 s, it is where the old version started.)
static Vec3 attachedPosAt(float w)
{
    float th = mainHeadThetaAt(w);
    return mainArcPoint(th) + arcTangentBack(th) * 4.0f + pathSide() * (-3.0f) + Vec3(0.0f, -1.2f, 0.0f);
}

static Vec3 bezier3(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, float u)
{
    float v  = 1.0f - u;
    float b0 = v * v * v, b1 = 3.0f * v * v * u, b2 = 3.0f * v * u * u, b3 = u * u * u;
    return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
}

// Path of the derailed meteor (no loops, no spiral: only smoothstep + one cubic Bezier + a line):
//   w <= 5.0      Stage 1  attached to the main meteor
//   5.0 .. 8.0    Stage 2  smooth separation: pos = attached*(1-k) + curve*k, k = smoothstep(5, 8, w)
//                          k starts at 0 with zero slope, so the speed and direction at 5 s are
//                          exactly those of the main meteor
//   5.0 .. 11.5   Stage 3  cubic Bezier from the break-away point P0 to the approach point Q:
//                          P1 = P0 + 16 m along the travel direction (smooth start)
//                          P2 = Q - 12 m along the final approach direction (smooth end)
//   11.5 .. 14.5  Stage 4  straight line from Q to the landing point, gently accelerating
//                          (the line has the direction the Bezier ends with, and about the same speed,
//                           so there is no jerk at 11.5 s)
//   w >= 14.5     Stage 5  at the landing point (the explosion starts)
static Vec3 derailedPosAt(float w)
{
    Vec3 att = attachedPosAt(w);
    if (w <= T_SEP) return att;

    Vec3 L = landingPoint();
    Vec3 Q(APPROACH_X, APPROACH_Y, APPROACH_Z);

    if (w >= T_CURVE_END)
    {
        float sg = clampf((w - T_CURVE_END) / (T_HIT - T_CURVE_END), 0.0f, 1.0f);
        float e  = 0.9f * sg + 0.1f * sg * sg;               // almost constant speed, a little faster at the end
        return Q + (L - Q) * e;
    }

    Vec3 p0   = attachedPosAt(T_SEP);
    Vec3 vdir = norm3(attachedPosAt(T_SEP + 0.05f) - attachedPosAt(T_SEP - 0.05f));
    Vec3 p1   = p0 + vdir * 16.0f;
    Vec3 adir = norm3(L - Q);
    Vec3 p2   = Q - adir * 12.0f;

    float u     = (w - T_SEP) / (T_CURVE_END - T_SEP);
    Vec3 curve  = bezier3(p0, p1, p2, Q, u);
    float k     = smoothstep(T_SEP, T_SEP_END, w);
    return att * (1.0f - k) + curve * k;
}

// Size of the derailed meteor head: grows slowly, then quickly during the straight approach.
static float derailedScaleAt(float w)
{
    float sg = clampf((w - T_CURVE_END) / (T_HIT - T_CURVE_END), 0.0f, 1.0f);
    return 1.0f + 1.5f * smoothstep(T_SEP, T_CURVE_END, w) + 9.0f * sg * sg;
}

// 1 before the impact, 0 shortly after: the meteors disappear into the explosion.
static float impactFadeAt(float w) { return 1.0f - smoothstep(T_HIT, T_HIT + 0.8f, w); }





static float derailedBrightAt(float w) { return 0.75f + 0.25f * smoothstep(T_SEP, T_HIT, w); }

static float mainPulseAt(float w)   { return 1.0f + 0.12f * sinf(w * 6.0f) + 0.04f * sinf(w * 15.0f); }
static float derailPulseAt(float w) { return 1.0f + 0.15f * sinf(w * 8.0f + 1.0f); }

// How strong the explosion light / flash is, 0..1. It uses the REAL timeline.
static float flashLevel()
{
    return smoothstep(T_HIT, T_FULL_WHITE, sceneTime)
         * (1.0f - smoothstep(T_RESET, sceneDuration, sceneTime));
}

// ============================================================================
//  UPDATE FUNCTIONS  (none of them knows where the camera looks)
// ============================================================================

// ----------------------------------------------------------------------------
//  updateGlobalSceneTime: the global clock.
//   1. apply mouse-wheel movement smoothly   2. let the clock run (unless SPACE paused)
//   3. loop seamlessly at the end            4. compute worldTime for the sky objects
//  At T_RESET (18.4 s) the whole screen is white, so the sky restarts underneath.
//  sceneTime - 18.4 gives exactly the same worldTime as the wrapped value, so
//  the loop has no visible jump.
// ----------------------------------------------------------------------------
static void updateGlobalSceneTime(float dt)
{
    float take = g_scrubPending * (1.0f - expf(-14.0f * dt));
    if (fabsf(g_scrubPending) < 0.002f) take = g_scrubPending;
    g_scrubPending -= take;

    float moved = sceneTime + take;
    if (moved < 0.0f || moved > sceneDuration) g_scrubPending = 0.0f;
    sceneTime = clampf(moved, 0.0f, sceneDuration);

    if (!g_paused)
    {
        sceneTime += dt;
        if (sceneTime >= sceneDuration) sceneTime -= T_RESET;
    }
    sceneTime = clampf(sceneTime, 0.0f, sceneDuration);

    g_worldTime = (sceneTime >= T_RESET) ? (sceneTime - T_RESET) : sceneTime;
}

// ----------------------------------------------------------------------------
//  [MODIFIED] updateMainComet
//  The head moves along the 0 -> 180 degree arc. The TAIL is rebuilt in world space:
//  80 samples along the arc from the head backward (75 degrees of arc). Each sample gets
//  two crossed ribbons (one in the path plane, one across it), so the glowing tail is
//  visible from ANY camera angle. The 7 coloured layers are unchanged. Samples that are
//  below the horizon fade out (smoothstep on their height), so the tail rises out of the
//  horizon at the start and sets over the front horizon at the end. The main meteor never
//  lands. The geometry goes into a VBO.
// ----------------------------------------------------------------------------
static void updateMainComet()
{
    float w = g_worldTime;
    g_mainTheta = mainHeadThetaAt(w);
    g_mainHead  = mainArcPoint(g_mainTheta);
    g_mainPulse = mainPulseAt(w);

    static const LayerDef layers[] = {
        { 0.10f, 0.30f, 1.00f, 0.16f, 2.00f,  0.00f },   // wide transparent blue glow
        { 0.15f, 0.40f, 1.00f, 0.45f, 1.05f,  0.20f },   // blue body
        { 0.45f, 0.75f, 1.00f, 0.50f, 0.35f,  0.60f },   // light blue
        { 0.10f, 0.95f, 1.00f, 0.55f, 0.30f,  0.10f },   // cyan
        { 0.60f, 0.28f, 1.00f, 0.60f, 0.40f, -0.45f },   // purple
        { 1.00f, 0.25f, 0.75f, 0.65f, 0.22f, -0.82f },   // magenta edge
        { 0.85f, 0.95f, 1.00f, 0.95f, 0.09f,  0.85f }    // white-blue core
    };
    const int N = 80;
    Vec3  S1   = pathSide();

    g_tailVerts.clear();
    g_tailRanges.clear();
    for (int ribbon = 0; ribbon < 2; ++ribbon)
        for (size_t li = 0; li < sizeof(layers) / sizeof(layers[0]); ++li)
        {
            const LayerDef &d = layers[li];
            std::vector<Vtx3> top, mid, bot;
            for (int i = 0; i <= N; ++i)
            {
                float t    = (float)i / (float)N;
                float th   = g_mainTheta - t * TAIL_ARC;            // going backward along the arc
                float prof = 0.012f + 0.988f * powf(t, 1.2f);       // thin at the head, wide far away
                Vec3  c    = mainArcPoint(th);
                Vec3  T    = arcTangentBack(th);
                Vec3  S    = (ribbon == 0) ? S1 : norm3(cross3(S1, T));
                float hw   = MAIN_MAX_HW * d.w * prof * TAIL_SCALE;
                float off  = d.off * MAIN_MAX_HW * prof * TAIL_SCALE;
                Vec3  cc   = c + S * off;
                float hv   = smoothstep(2.0f, 10.0f, c.y);          // hidden below the horizon
                float a    = d.a * (0.25f + 0.75f * smoothstep(0.0f, 0.35f, t))
                                 * (1.0f - smoothstep(0.88f, 1.0f, t)) * hv
                                 * (ribbon == 0 ? 1.0f : 0.7f);
                top.push_back(make3(cc + S * hw, d.r, d.g, d.b, 0.0f));
                mid.push_back(make3(cc,          d.r, d.g, d.b, a));
                bot.push_back(make3(cc - S * hw, d.r, d.g, d.b, 0.0f));
            }
            pushStrips3(g_tailVerts, g_tailRanges, top, mid, bot);
        }

    g_tailBuf.data.assign((const float *)&g_tailVerts[0],
                          (const float *)&g_tailVerts[0] + g_tailVerts.size() * 7);
    refreshBuffer(g_tailBuf);
}

// ----------------------------------------------------------------------------
//  [MODIFIED] updateDerailedMeteor: position from derailedPosAt() (see the stages
//  there). The direction of travel is the slope of the path (two nearby samples), so
//  everything that is oriented follows the path and nothing spins by itself.
// ----------------------------------------------------------------------------
static void updateDerailedMeteor()
{
    float w = g_worldTime;
    g_derailPos = derailedPosAt(w);

    const float h = 0.03f;
    Vec3 vel = (derailedPosAt(w + h) - derailedPosAt(w - h)) * (1.0f / (2.0f * h));
    if (dot3(vel, vel) > 0.04f)                 // keep the old direction when almost standing still
        g_derailDir = norm3(vel);

    g_derailScale  = derailedScaleAt(w);
    g_derailBright = derailedBrightAt(w);
}

// Extra meteors: flight number and position come from the timeline + a repeatable hash.
// Each flight starts at the top or the right edge of the meteor sky region and flies
// down-left. The meteor grows from nothing at the start and shrinks away at the end
// (fade), so it never pops in or out.
static void updateExtraMeteors()
{
    for (size_t i = 0; i < g_extra.size(); ++i)
    {
        ExtraMeteor &m = g_extra[i];

        float total = g_worldTime + m.phase;
        int   n     = (int)floorf(total / m.period);
        float local = total - (float)n * m.period;

        float r1 = hash3((int)i, n, 1);
        float r2 = hash3((int)i, n, 2);
        float r3 = hash3((int)i, n, 3);
        float r4 = hash3((int)i, n, 4);

        float ang = m.baseAngle + (r4 - 0.5f) * 8.0f;
        float dx  = cosf(deg2rad(ang));
        float dy  = sinf(deg2rad(ang));
        float margin = 30.0f;

        float x0, y0;
        if (r1 < 0.55f)
        {
            x0 = lerpf(SKY_L + 0.35f * (SKY_R - SKY_L), SKY_R + margin, r2);
            y0 = SKY_T + margin;
        }
        else
        {
            x0 = SKY_R + margin;
            y0 = lerpf(SKY_B + 0.30f * (SKY_T - SKY_B), SKY_T, r3);
        }

        float tx = (dx < 0.0f) ? (SKY_L - margin - x0) / dx : 1.0e9f;
        float ty = (dy < 0.0f) ? (SKY_B - margin - y0) / dy : 1.0e9f;
        float tExit = std::min(tx, ty) / m.speed;

        m.active     = (local < tExit);
        m.x          = x0 + dx * m.speed * local;
        m.y          = y0 + dy * m.speed * local;
        m.trailAngle = ang + 180.0f;
        m.fade       = smoothstep(0.0f, 0.25f, local) * (1.0f - smoothstep(tExit - 0.35f, tExit, local));
    }
}

static void updateMeteorAnimation()
{
    updateMainComet();
    updateDerailedMeteor();
    updateExtraMeteors();
    updateStars();
}

// ----------------------------------------------------------------------------
//  [MODIFIED] updateMeteorLighting  -  the meteors light the WHOLE environment.
//  It uses only the meteors' world positions and the timeline. It is called every
//  frame, whether or not any meteor is inside the camera's view.
//  Must be called right after gluLookAt: light positions are given in world
//  coordinates and OpenGL transforms them by the current modelview matrix.
// ----------------------------------------------------------------------------
static void setLightColor(GLenum light, float r, float g, float b, float amb, float spec)
{
    GLfloat d[] = { r, g, b, 1.0f };
    GLfloat a[] = { r * amb, g * amb, b * amb, 1.0f };
    GLfloat s[] = { r * spec, g * spec, b * spec, 1.0f };
    glLightfv(light, GL_DIFFUSE,  d);
    glLightfv(light, GL_AMBIENT,  a);
    glLightfv(light, GL_SPECULAR, s);
}
static void setLightPos(GLenum light, float x, float y, float z, float w)
{
    GLfloat p[] = { x, y, z, w };
    glLightfv(light, GL_POSITION, p);
}

// [NEW] applyMeteorGlow: the colour that the approaching meteors give to the camp.
//   - LIGHT7 is a spot light above the landing point, aimed at the FRONT of the tent
//     (the door side) and the ground in front of it. It grows from violet to white
//     while the meteors approach (stage 3 and 4).
//   - the global ambient light stays a dark navy blue and only gets a faint violet
//     lift during the approach, and a big lift during the white flash.
static void applyMeteorGlow(float flash)
{
    float w = g_worldTime;
    float approach = smoothstep(8.0f, T_HIT, w) * impactFadeAt(w);       // 0 .. 1

    float ta = deg2rad(TENT_ANGLE);
    Vec3 target(TENT_X + sinf(ta) * 2.3f, 0.9f, TENT_Z + cosf(ta) * 2.3f);   // tent door centre
    Vec3 spotPos(LANDING_X + 2.0f, 9.0f, LANDING_Z + 2.0f);
    Vec3 dir = target - spotPos;
    GLfloat sp[] = { spotPos.x, spotPos.y, spotPos.z, 1.0f };
    GLfloat sd[] = { dir.x, dir.y, dir.z };
    glLightfv(GL_LIGHT7, GL_POSITION, sp);
    glLightfv(GL_LIGHT7, GL_SPOT_DIRECTION, sd);
    float k = 2.2f * approach;
    setLightColor(GL_LIGHT7, lerpf(0.55f, 0.90f, approach) * k, lerpf(0.35f, 0.90f, approach) * k,
                  1.00f * k, 0.0f, 0.5f);

    GLfloat amb[] = { 0.12f + 0.10f * approach + 0.9f * flash,
                      0.15f + 0.06f * approach + 0.9f * flash,
                      0.30f + 0.18f * approach + 0.9f * flash, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
}

static void updateMeteorLighting()
{
    float w     = g_worldTime;
    float t     = sceneTime;
    float flash = flashLevel();
    float vis   = impactFadeAt(w);

    // --- main meteor head: cyan-white-blue. It lights the camp from the direction it is in:
    //     the ground gets most light when the meteor is high, the tent sides when it is low. ---
    float mvis = smoothstep(2.0f, 12.0f, g_mainHead.y);          // the main meteor never "lands"
    float k0 = mvis * g_mainPulse;
    setLightPos  (GL_LIGHT0, g_mainHead.x, g_mainHead.y, g_mainHead.z, 1.0f);
    setLightColor(GL_LIGHT0, 0.50f * k0, 0.80f * k0, 1.40f * k0, 0.0f, 0.5f);

    // --- a point 40 degrees back along the tail: violet-magenta light ---
    Vec3 tp = mainArcPoint(g_mainTheta - 40.0f);
    setLightPos  (GL_LIGHT1, tp.x, tp.y, tp.z, 1.0f);
    setLightColor(GL_LIGHT1, 0.75f * k0, 0.40f * k0, 1.10f * k0, 0.0f, 0.4f);

    // --- derailed meteor: pink -> white, stronger as it grows and nears the landing point ---
    float m  = smoothstep(T_CURVE_END - 2.0f, T_HIT, w);
    float k2 = clampf(0.25f + 0.10f * g_derailScale, 0.0f, 1.4f) * vis * smoothstep(2.0f, 9.0f, g_derailPos.y);
    setLightPos  (GL_LIGHT2, g_derailPos.x, g_derailPos.y, g_derailPos.z, 1.0f);
    setLightColor(GL_LIGHT2, 1.00f * k2, (0.55f + 0.45f * m) * k2, (0.70f + 0.30f * m) * k2, 0.0f, 0.5f);

    // --- campfire: warm, flickering, only reaches the camp (a bit weaker than before) ---
    float flick = 1.2f + 0.2f * sinf(fireTime * 9.0f) + 0.12f * sinf(fireTime * 23.0f + 1.0f);
    setLightPos  (GL_LIGHT3, 0.0f, 1.0f, -5.0f, 1.0f);
    setLightColor(GL_LIGHT3, 1.00f * flick, 0.55f * flick, 0.22f * flick, 0.0f, 0.4f);

    // --- meteor-shower light: a faint violet-blue light from the sky, a little stronger
    //     while the main meteor is near the camp ---
    float overhead = clampf((g_mainHead.y - CAM_Y) / MAIN_R, 0.0f, 1.0f);   // 1 = straight overhead
    float ks   = 0.85f + 0.5f * overhead * mvis;
    setLightPos  (GL_LIGHT4, 0.35f, 0.85f, 0.45f, 0.0f);
    setLightColor(GL_LIGHT4, 0.16f * ks, 0.14f * ks, 0.34f * ks, 0.0f, 0.0f);

    // --- flash light: white, from above, only during the white-out ---
    setLightPos  (GL_LIGHT5, 0.0f, 1.0f, 0.3f, 0.0f);
    setLightColor(GL_LIGHT5, 1.4f * flash, 1.4f * flash, 1.4f * flash, 0.0f, 0.3f);

    // --- explosion light: starts AT THE LANDING POINT; its reach R grows with time, because
    //     the quadratic attenuation is 4 / R^2 (R = 3 m at the impact, about 73 m at full white) ---
    float e    = smoothstep(T_HIT, T_FULL_WHITE, t);
    float rise = smoothstep(T_HIT, T_HIT + 0.8f, t);
    float fade = 1.0f - smoothstep(T_FULL_WHITE, T_RESET, t);
    float I    = 3.0f * rise * fade;
    float R    = 3.0f + 70.0f * e;
    setLightPos  (GL_LIGHT6, LANDING_X, LANDING_Y + 1.2f, LANDING_Z, 1.0f);
    setLightColor(GL_LIGHT6, 1.00f * I, 0.85f * I, 0.70f * I, 0.0f, 0.6f);
    glLightf(GL_LIGHT6, GL_QUADRATIC_ATTENUATION, 4.0f / (R * R));

    // --- spot light on the tent front + ambient (also adds the flash to the ambient) ---
    applyMeteorGlow(flash);

    // --- fog colour: dark navy, turning white during the flash ---
    GLfloat fog[] = { lerpf(0.02f, 1.0f, flash), lerpf(0.03f, 1.0f, flash), lerpf(0.08f, 1.0f, flash), 1.0f };
    glFogfv(GL_FOG_COLOR, fog);
}

// ----------------------------------------------------------------------------
//  Camera
// ----------------------------------------------------------------------------
static void updateCameraDirection()
{
    frontX = cosf(deg2rad(yaw)) * cosf(deg2rad(pitch));
    frontY = sinf(deg2rad(pitch));
    frontZ = sinf(deg2rad(yaw)) * cosf(deg2rad(pitch));
}

static void updateCamera(float dt)
{
    // arrow keys: look (held keys, frame-rate independent)
    if (leftArrow)  yaw   -= LOOK_RATE * dt;
    if (rightArrow) yaw   += LOOK_RATE * dt;
    if (upArrow)    pitch += LOOK_RATE * dt;
    if (downArrow)  pitch -= LOOK_RATE * dt;
    pitch = clampf(pitch, -89.0f, 89.0f);
    updateCameraDirection();

    // WASD: walk (same directions as the campsite program)
    float speed  = MOVE_SPEED * dt;
    float rightX = -sinf(deg2rad(yaw));
    float rightZ =  cosf(deg2rad(yaw));
    if (wPressed) { camX += frontX * speed; camZ += frontZ * speed; }
    if (sPressed) { camX -= frontX * speed; camZ -= frontZ * speed; }
    if (aPressed) { camX -= rightX * speed; camZ -= rightZ * speed; }
    if (dPressed) { camX += rightX * speed; camZ += rightZ * speed; }
}

// ============================================================================
//  DRAWING: SKY
// ============================================================================

// Sky dome gradient + far ridge. Always centred on the camera, so it never gets closer.
static void drawNightSky()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(camX, camY, camZ);

    const char *base = useBuffer(g_skyBuf);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 7 * sizeof(float), base);
    glColorPointer(4, GL_FLOAT, 7 * sizeof(float), base + 3 * sizeof(float));
    for (size_t i = 0; i < g_domeRanges.size(); ++i)
        glDrawArrays(g_domeRanges[i].mode, g_domeRanges[i].first, g_domeRanges[i].count);
    for (size_t i = 0; i < g_ridgeRanges.size(); ++i)          // far layer first, near layer on top
        glDrawArrays(g_ridgeRanges[i].mode, g_ridgeRanges[i].first, g_ridgeRanges[i].count);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    releaseBuffer();

    glPopMatrix();
}

static void drawStars()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(camX, camY, camZ);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    for (int s = 0; s < NUM_STAR_SETS; ++s)
    {
        StarSet &set = g_stars[s];
        if (set.base.empty()) continue;
        glPointSize(set.size);
        glVertexPointer(3, GL_FLOAT, 0, &set.pos[0]);
        glColorPointer(4, GL_FLOAT, 0, &set.col[0]);
        glDrawArrays(GL_POINTS, 0, (GLsizei)set.base.size());
    }
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

// [NEW] Translucent atmospheric layers: big, very faint coloured glows on the sky dome
// (violet, cyan, magenta, blue). They breathe slowly with the global time and always
// face the camera (billboardRotation). Additive blending keeps them subtle.
static void drawAtmosphere()
{
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_glowTex);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (size_t i = 0; i < g_hazes.size(); ++i)
    {
        const Haze &h = g_hazes[i];
        float pulse = 0.75f + 0.25f * sinf(g_worldTime * 0.15f + h.phase);
        glPushMatrix();
        glTranslatef(camX + 150.0f * h.dx, camY + 150.0f * h.dy, camZ + 150.0f * h.dz);
        billboardRotation();
        drawTexturedQuad(h.size, h.r, h.g, h.b, h.a * pulse);
        glPopMatrix();
    }
    glDisable(GL_TEXTURE_2D);
}

static void drawCloudPuffs(const Cloud &cl)
{
    float k = cl.alphaScale;
    for (int pass = 0; pass < 2; ++pass)               // pass 0 = dark body, pass 1 = lit highlights
        for (size_t i = 0; i < cl.puffs.size(); ++i)
        {
            const Puff &p = cl.puffs[i];
            glPushMatrix();
            if (pass == 0)
            {
                glTranslatef(p.ox, p.oy, 0.0f);
                glRotatef(p.rot, 0.0f, 0.0f, 1.0f);
                glScalef(p.r * 1.15f, p.r * 0.85f, 1.0f);
                drawTexturedQuad(1.0f, 0.36f * p.shade, 0.46f * p.shade, 0.66f * p.shade, 0.82f * k);
            }
            else
            {
                glTranslatef(p.ox - 0.25f * p.r, p.oy - 0.20f * p.r, 0.0f);
                glRotatef(p.rot, 0.0f, 0.0f, 1.0f);
                glScalef(p.r * 0.70f, p.r * 0.55f, 1.0f);
                drawTexturedQuad(1.0f, 0.70f, 0.80f, 0.98f, 0.50f * k);
            }
            glPopMatrix();
        }
}

static void beginCloudState()
{
    glEnable(GL_LIGHTING);                             // the meteor lights colour the clouds
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_cloudTex);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static void endCloudState()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

// Billboard clouds around the sky: each faces the origin: glRotatef(-azimuth, 0,1,0)
static void drawBandList(const std::vector<Cloud> &list, float drift)
{
    for (size_t c = 0; c < list.size(); ++c)
    {
        const Cloud &cl = list[c];
        float dx = sinf(g_worldTime * 0.10f + cl.phase) * drift;       // slow drift (metres)
        float dy = sinf(g_worldTime * 0.07f + cl.phase * 1.7f) * 0.4f;
        glPushMatrix();
        glTranslatef(cl.x + dx, cl.y + dy, cl.z);
        glRotatef(-cl.az, 0.0f, 1.0f, 0.0f);
        drawCloudPuffs(cl);
        glPopMatrix();
    }
}

// [MODIFIED] distant clouds all around the sky (high clouds first, low band clouds in front)
static void drawClouds()
{
    beginCloudState();
    drawBandList(g_highClouds, 5.0f);
    drawBandList(g_bandClouds, 3.0f);
    endCloudState();
}

// The cloud group of the original meteor scene (north panel).
static void drawPanelClouds()
{
    beginCloudState();
    beginPanel(0.0f);
    for (size_t c = 0; c < g_panelClouds.size(); ++c)
    {
        const Cloud &cl = g_panelClouds[c];
        float dx = sinf(g_worldTime * 0.12f + cl.phase) * 30.0f;     // slow drift (panel pixels)
        float dy = sinf(g_worldTime * 0.09f + cl.phase * 1.7f) * 8.0f;
        glPushMatrix();
        glTranslatef(cl.x + dx, cl.y + dy, 0.0f);
        drawCloudPuffs(cl);
        glPopMatrix();
    }
    endPanel();
    endCloudState();
}

// ----------------------------------------------------------------------------
//  Fast extra meteors (sky pass). Every meteor is drawn in the panel of its own
//  compass direction (beginPanel), then translated, rotated and scaled (fade).
// ----------------------------------------------------------------------------
static void drawAdditionalMeteors()
{
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (size_t i = 0; i < g_extra.size(); ++i)
    {
        const ExtraMeteor &m = g_extra[i];
        if (!m.active || m.fade < 0.02f) continue;

        beginPanel(m.panelAz);
        glPushMatrix();
        glTranslatef(m.x, m.y, 0.0f);
        glRotatef(m.trailAngle, 0.0f, 0.0f, 1.0f);
        glScalef(m.fade, m.fade, 1.0f);                // grows in / shrinks away

        glDisable(GL_TEXTURE_2D);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        const char *base = (const char *)&m.trail[0];
        glVertexPointer(2, GL_FLOAT, sizeof(Vtx), base);
        glColorPointer(4, GL_FLOAT, sizeof(Vtx), base + 2 * sizeof(float));
        for (size_t k = 0; k < m.ranges.size(); ++k)
            glDrawArrays(m.ranges[k].mode, m.ranges[k].first, m.ranges[k].count);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_glowTex);
        drawTexturedQuad(m.headSize * 2.2f, m.r, m.g, m.b, m.alpha * 0.55f);
        drawTexturedQuad(m.headSize * 0.8f, 1.0f, 1.0f, 1.0f, m.alpha);
        glDisable(GL_TEXTURE_2D);

        glPopMatrix();
        endPanel();
    }
}

// ----------------------------------------------------------------------------
//  [MODIFIED] drawMainMeteor: the tail from the VBO. It is drawn in WORLD space in the
//  glow pass (depth TEST on, depth WRITES off), so trees and tents hide it correctly and it
//  is visible from every camera direction. It does not depend on the camera at all.
// ----------------------------------------------------------------------------
static void drawMainMeteor()
{
    if (g_tailBuf.data.empty()) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    const char *base = useBuffer(g_tailBuf);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 7 * sizeof(float), base);
    glColorPointer(4, GL_FLOAT, 7 * sizeof(float), base + 3 * sizeof(float));
    for (size_t i = 0; i < g_tailRanges.size(); ++i)
        glDrawArrays(g_tailRanges[i].mode, g_tailRanges[i].first, g_tailRanges[i].count);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    releaseBuffer();
    glPopMatrix();
}

// ----------------------------------------------------------------------------
//  [MODIFIED] drawDerailedMeteor: its trail follows the SAME curved path:
//  48 earlier positions of derailedPosAt() form a ribbon. Each ribbon point is
//  widened sideways (perpendicular to the path and to the view direction), so the
//  ribbon always faces the viewer. The width and brightness grow with the head.
// ----------------------------------------------------------------------------
static void drawDerailedMeteor()
{
    float w = g_worldTime;
    const int   N    = 48;
    const float SPAN = 2.4f;                        // seconds of the path shown as trail
    Vec3 hist[N + 1];
    for (int k = 0; k <= N; ++k)
        hist[k] = derailedPosAt(w - (float)k * SPAN / (float)N);

    float fade      = impactFadeAt(w);
    float widthBase = 0.12f + 0.10f * powf(g_derailScale, 0.8f);
    float whiten    = smoothstep(T_CURVE_END - 2.0f, T_HIT, w);
    Vec3  cam(camX, camY, camZ);

    std::vector<Vtx3> top, mid, bot;
    Vec3 side(0.0f, 1.0f, 0.0f);
    for (int k = 0; k <= N; ++k)
    {
        Vec3 a   = hist[k > 0 ? k - 1 : k];
        Vec3 b   = hist[k < N ? k + 1 : k];
        Vec3 sd  = cross3(b - a, hist[k] - cam);
        if (dot3(sd, sd) > 1e-8f) side = norm3(sd);

        float t  = (float)k / (float)N;
        float hw = widthBase * powf(1.0f - t, 0.8f);
        float al = clampf(g_derailBright * 0.9f * powf(1.0f - t, 1.2f), 0.0f, 1.0f) * fade
                 * smoothstep(2.0f, 9.0f, hist[k].y);
        float cr = lerpf(1.00f, 0.60f, t);
        float cg = lerpf(0.85f, 0.80f, t);
        float cb = lerpf(0.95f, 1.00f, t);
        cr = lerpf(cr, 1.0f, whiten * (1.0f - t));
        cg = lerpf(cg, 1.0f, whiten * (1.0f - t));
        cb = lerpf(cb, 1.0f, whiten * (1.0f - t));

        top.push_back(make3(hist[k] + side * hw, cr, cg, cb, 0.0f));
        mid.push_back(make3(hist[k],             cr, cg, cb, al));
        bot.push_back(make3(hist[k] - side * hw, cr, cg, cb, 0.0f));
    }
    g_derailVerts.clear();
    g_derailRanges.clear();
    pushStrips3(g_derailVerts, g_derailRanges, top, mid, bot);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPushMatrix();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    const char *base = (const char *)&g_derailVerts[0];
    glVertexPointer(3, GL_FLOAT, sizeof(Vtx3), base);
    glColorPointer(4, GL_FLOAT, sizeof(Vtx3), base + 3 * sizeof(float));
    for (size_t i = 0; i < g_derailRanges.size(); ++i)
        glDrawArrays(g_derailRanges[i].mode, g_derailRanges[i].first, g_derailRanges[i].count);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

// ----------------------------------------------------------------------------
//  Glow pass (drawn AFTER the campsite, depth test ON, depth writes OFF): the bright
//  heads of the two meteors and the explosion. Because of the depth test, trees and
//  tents hide them correctly. Heads are camera-facing sprites placed at world positions.
// ----------------------------------------------------------------------------
static void drawMeteorGlows()
{
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_glowTex);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // ---- main meteor head ----
    {
        float fade = smoothstep(2.0f, 10.0f, g_mainHead.y);       // no landing: it sets behind the horizon
        float p    = g_mainPulse;
        glPushMatrix();
        glTranslatef(g_mainHead.x, g_mainHead.y, g_mainHead.z);
        billboardRotation();
        drawTexturedQuad(4.5f * p, 0.15f, 0.35f, 1.00f, 0.35f * fade);   // big blue halo
        drawTexturedQuad(2.2f * p, 0.30f, 0.70f, 1.00f, 0.60f * fade);   // cyan glow
        drawTexturedQuad(1.0f * p, 0.70f, 0.95f, 1.00f, 0.90f * fade);   // bright inner glow
        drawTexturedQuad(0.40f * p, 1.00f, 1.00f, 1.00f, 1.00f * fade);  // white core
        glPushMatrix();                                                  // flare across the screen
        glScalef(3.0f, 0.12f, 1.0f);
        drawTexturedQuad(1.8f * p, 0.60f, 0.85f, 1.0f, 0.70f * fade);
        glPopMatrix();
        glPushMatrix();
        glScalef(0.12f, 2.0f, 1.0f);
        drawTexturedQuad(1.1f * p, 0.60f, 0.85f, 1.0f, 0.45f * fade);
        glPopMatrix();
        glPopMatrix();
    }

    // ---- derailed meteor head: translate (position) -> face the viewer -> scale (size) ----
    {
        float fade = impactFadeAt(g_worldTime) * smoothstep(2.0f, 9.0f, g_derailPos.y);
        float S = g_derailScale * derailPulseAt(g_worldTime);
        float B = g_derailBright;
        float m = smoothstep(T_CURVE_END - 2.0f, T_HIT, g_worldTime);   // 0 = pink head, 1 = white head
        glPushMatrix();
        glTranslatef(g_derailPos.x, g_derailPos.y, g_derailPos.z);
        billboardRotation();
        glScalef(S, S, 1.0f);                                           // grows toward the landing
        drawTexturedQuad(1.40f, 1.0f, 0.35f + 0.65f * m, 0.55f + 0.45f * m, 0.45f * B * fade);   // halo
        drawTexturedQuad(0.60f, 1.0f, 0.60f + 0.40f * m, 0.75f + 0.25f * m, 0.85f * B * fade);   // inner glow
        drawTexturedQuad(0.28f, 1.0f, 0.95f, 0.97f, 1.0f * fade);                                // white core
        glPopMatrix();
    }

    drawExplosionLight();
    glDisable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------------------------
//  [NEW] drawExplosionLight: the visible part of the explosion. It starts exactly at
//  the landing point (LANDING_X/Y/Z) at T_HIT = 14.5 s and uses sceneTime:
//    1. a glow sphere (camera-facing sprite) whose radius grows from 0.4 m to 70 m
//    2. a bright ring on the ground that spreads outward (a shock wave)
//  Both fade away between 17.5 s and 18.4 s, while the full-screen white takes over.
//  The matching light (LIGHT6) is set in updateMeteorLighting().
// ----------------------------------------------------------------------------
static void drawExplosionLight()
{
    float t    = sceneTime;
    float e    = smoothstep(T_HIT, T_FULL_WHITE, t);
    float rise = smoothstep(T_HIT, T_HIT + 1.0f, t);
    float fade = 1.0f - smoothstep(T_FULL_WHITE, T_RESET, t);
    if (rise * fade < 0.001f) return;

    // 1) the expanding glow sphere
    float radius = 0.4f + 70.0f * e * e;
    glPushMatrix();
    glTranslatef(LANDING_X, LANDING_Y + 0.4f, LANDING_Z);
    billboardRotation();
    drawTexturedQuad(radius,        0.85f, 0.92f, 1.0f, 0.90f * rise * fade);
    drawTexturedQuad(radius * 0.5f, 1.00f, 1.00f, 1.0f, 1.00f * rise * fade);
    glPopMatrix();

    // 2) the ground ring (vertex array, flat, additive)
    glDisable(GL_TEXTURE_2D);
    float rr = 1.0f + 38.0f * e;                       // ring radius (metres)
    float wd = 0.8f + 2.5f * e;                        // ring half width
    float ra = 0.9f * rise * fade * (1.0f - 0.7f * e);
    float y  = LANDING_Y + 0.06f;
    const int SEG = 72;
    std::vector<Vtx3> top, mid, bot;
    for (int i = 0; i <= SEG; ++i)
    {
        float a  = 2.0f * PI * (float)i / (float)SEG;
        float cx = cosf(a), sz = sinf(a);
        float ri = std::max(rr - wd, 0.05f);
        top.push_back(make3(Vec3(LANDING_X + cx * (rr + wd), y, LANDING_Z + sz * (rr + wd)), 0.90f, 0.95f, 1.0f, 0.0f));
        mid.push_back(make3(Vec3(LANDING_X + cx * rr,        y, LANDING_Z + sz * rr),        1.00f, 0.90f, 0.8f, ra));
        bot.push_back(make3(Vec3(LANDING_X + cx * ri,        y, LANDING_Z + sz * ri),        0.90f, 0.95f, 1.0f, 0.0f));
    }
    std::vector<Vtx3>  verts;
    std::vector<Range> ranges;
    pushStrips3(verts, ranges, top, mid, bot);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    const char *base = (const char *)&verts[0];
    glVertexPointer(3, GL_FLOAT, sizeof(Vtx3), base);
    glColorPointer(4, GL_FLOAT, sizeof(Vtx3), base + 3 * sizeof(float));
    for (size_t i = 0; i < ranges.size(); ++i)
        glDrawArrays(ranges[i].mode, ranges[i].first, ranges[i].count);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}


// ----------------------------------------------------------------------------
//  Flash, part 2 (foreground): a full-screen white layer.
//    15.5 s -> 17.5 s : fade IN      17.5 s -> 18.4 s : completely white
//    18.4 s -> 20.0 s : fade OUT (the sky is already restarted underneath)
//  The campsite is lit up by the flash light BEFORE this layer shows up
//  (see updateMeteorLighting). It is drawn in a temporary 2D projection.
// ----------------------------------------------------------------------------
static void drawWhiteFlash()
{
    float a = smoothstep(T_FADE_IN_START, T_FULL_WHITE, sceneTime)
            * (1.0f - smoothstep(T_RESET, sceneDuration, sceneTime));
    if (a < 0.001f) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, a);
    glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(1.0f, 0.0f);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(0.0f, 1.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
//  CAMPSITE  (from the campsite program)
//  Changes: no GLEW; every face now has a normal so fixed-function lighting works;
//  the flat per-face colour multipliers were replaced by real lighting; the ground
//  is a grid of 1 m tiles so the campfire and meteor light show on it.
//  Shapes, sizes, positions and base colours are unchanged.
// ============================================================================

// grid lines on the ground (as before)
static void setupPlane()
{
    std::vector<float> &v = planeBuf.data;
    for (int i = -20; i <= 20; i++)
    {
        v.push_back((float)i);  v.push_back(0.0f); v.push_back(-20.0f);
        v.push_back((float)i);  v.push_back(0.0f); v.push_back(20.0f);
        v.push_back(-20.0f);    v.push_back(0.0f); v.push_back((float)i);
        v.push_back(20.0f);     v.push_back(0.0f); v.push_back((float)i);
    }
    planeVertexCount = (int)(v.size() / 3);
    uploadBuffer(planeBuf);
}

static void drawPlane()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.42f, 0.46f, 0.52f);
    const void *p = useBuffer(planeBuf);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, p);
    glDrawArrays(GL_LINES, 0, planeVertexCount);
    glDisableClientState(GL_VERTEX_ARRAY);
    releaseBuffer();
    glEnable(GL_LIGHTING);
}

// ground tiles: 40 x 40 grass, then the lighter 24 x 24 clearing on top
static void addTiles(std::vector<float> &v, int half, float y)
{
    for (int i = -half; i < half; ++i)
        for (int j = -half; j < half; ++j)
        {
            float x0 = (float)i, x1 = (float)(i + 1), z0 = (float)j, z1 = (float)(j + 1);
            float q[18] = { x0, y, z0,  x1, y, z0,  x1, y, z1,
                            x1, y, z1,  x0, y, z1,  x0, y, z0 };
            for (int k = 0; k < 18; ++k) v.push_back(q[k]);
        }
}
static void setupGround()
{
    std::vector<float> &v = groundBuf.data;
    addTiles(v, 20, -0.015f);
    groundGrassCount = (int)(v.size() / 3);
    addTiles(v, 12, -0.005f);
    groundClearCount = (int)(v.size() / 3) - groundGrassCount;
    uploadBuffer(groundBuf);
}

static void setupCube()
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
    cubeBuf.data.assign(cubeVertices, cubeVertices + sizeof(cubeVertices) / sizeof(float));
    uploadBuffer(cubeBuf);
}

static void setupPyramid()
{
    float pyramidVertices[] = {
        -0.5f, 0.0f, -0.5f,   0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f, 0.0f, -0.5f,   0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  -0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f, 0.0f,  0.5f,  -0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f
    };
    pyramidBuf.data.assign(pyramidVertices, pyramidVertices + sizeof(pyramidVertices) / sizeof(float));
    uploadBuffer(pyramidBuf);
}

static void setupPrism()
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
    prismBuf.data.assign(prismVertices, prismVertices + sizeof(prismVertices) / sizeof(float));
    uploadBuffer(prismBuf);
}

static float randomRange(float minValue, float maxValue)
{
    float t = rand() / (float)RAND_MAX;
    return minValue + t * (maxValue - minValue);
}

// random spots for the trees (runs once at the start, same rules as before)
static void setupTrees()
{
    srand(treeSeed);
    int tries = 0;
    while (treeCount < MAX_TREES && tries < 5000)
    {
        tries++;
        float x = randomRange(-19.0f, 19.0f);
        float z = randomRange(-19.0f, 19.0f);

        if (fabsf(x) < 12.0f && fabsf(z) < 12.0f) continue;      // not inside the clearing
        if (z < -15.0f) continue;                                // keep the cliff side open
        if (fabsf(x) < 4.5f && z > 12.0f) continue;              // keep the entrance path free

        bool tooClose = false;
        for (int i = 0; i < treeCount; i++)
        {
            float dx = x - treeX[i];
            float dz = z - treeZ[i];
            if (dx * dx + dz * dz < 9.0f) tooClose = true;
        }
        if (tooClose) continue;

        treeX[treeCount] = x;
        treeZ[treeCount] = z;
        treeScale[treeCount] = randomRange(0.8f, 1.3f);
        treeCount++;
    }
}

static void setupCampsite()
{
    setupPlane();
    setupGround();
    setupCube();
    setupPyramid();
    setupPrism();
    setupTrees();
}

// x, y, z is the bottom centre of the cube. Each face has its own normal.
static void drawCube(float x, float y, float z, float sx, float sy, float sz,
                     int r, int g, int b)
{
    glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(sx, sy, sz);

        const void *p = useBuffer(cubeBuf);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, p);

        glNormal3f( 0.0f, 0.0f, -1.0f); glDrawArrays(GL_TRIANGLES,  0, 6);   // back
        glNormal3f(-1.0f, 0.0f,  0.0f); glDrawArrays(GL_TRIANGLES,  6, 6);   // left
        glNormal3f( 1.0f, 0.0f,  0.0f); glDrawArrays(GL_TRIANGLES, 12, 6);   // right
        glNormal3f( 0.0f, 0.0f,  1.0f); glDrawArrays(GL_TRIANGLES, 18, 6);   // front
        glNormal3f( 0.0f, 1.0f,  0.0f); glDrawArrays(GL_TRIANGLES, 24, 6);   // top

        glDisableClientState(GL_VERTEX_ARRAY);
        releaseBuffer();
    glPopMatrix();
}

static void drawPyramid(float x, float y, float z, float sx, float sy, float sz,
                        int r, int g, int b)
{
    glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(sx, sy, sz);

        const void *p = useBuffer(pyramidBuf);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, p);

        glNormal3f( 0.0f, 0.5f, -1.0f); glDrawArrays(GL_TRIANGLES, 0, 3);
        glNormal3f( 1.0f, 0.5f,  0.0f); glDrawArrays(GL_TRIANGLES, 3, 3);
        glNormal3f( 0.0f, 0.5f,  1.0f); glDrawArrays(GL_TRIANGLES, 6, 3);
        glNormal3f(-1.0f, 0.5f,  0.0f); glDrawArrays(GL_TRIANGLES, 9, 3);

        glDisableClientState(GL_VERTEX_ARRAY);
        releaseBuffer();
    glPopMatrix();
}

// x, y, z is the bottom centre of the prism
static void drawPrism(float x, float y, float z, float sx, float sy, float sz,
                      int r, int g, int b)
{
    glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(sx, sy, sz);

        const void *p = useBuffer(prismBuf);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, p);

        glNormal3f(-1.0f, 0.5f,  0.0f); glDrawArrays(GL_TRIANGLES,  0, 6);   // left slope
        glNormal3f( 1.0f, 0.5f,  0.0f); glDrawArrays(GL_TRIANGLES,  6, 6);   // right slope
        glNormal3f( 0.0f, 0.0f,  1.0f); glDrawArrays(GL_TRIANGLES, 12, 3);   // front triangle
        glNormal3f( 0.0f, 0.0f, -1.0f); glDrawArrays(GL_TRIANGLES, 15, 3);   // back triangle

        glDisableClientState(GL_VERTEX_ARRAY);
        releaseBuffer();
    glPopMatrix();
}

// green grass for the whole plane, with a lighter green clearing in the middle
static void drawGround()
{
    const void *p = useBuffer(groundBuf);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, p);
    glNormal3f(0.0f, 1.0f, 0.0f);

    glColor3ub(90, 160, 70);                                      // grass
    glDrawArrays(GL_TRIANGLES, 0, groundGrassCount);
    glColor3ub(115, 180, 90);                                     // clearing around the spawn
    glDrawArrays(GL_TRIANGLES, groundGrassCount, groundClearCount);

    glDisableClientState(GL_VERTEX_ARRAY);
    releaseBuffer();
}

// the ground ends at z = -20, with a big drop after it
static void drawCliff()
{
    drawCube(0.0f, -0.2f, -20.2f, 40.0f, 0.2f, 0.4f, 110, 95, 70);        // rock edge
    drawCube(0.0f, -26.0f, -33.0f, 80.0f, 1.0f, 26.0f, 28, 60, 45);       // ground far below
}

// one flame piece (grows and shrinks with time). Flames glow: emission via glMaterialfv.
static void drawFlame(float x, float y, float z, float size, float speed, float phase,
                      int r, int g, int b)
{
    float s = 0.65f + 0.35f * sinf(fireTime * speed + phase);
    setEmission(r / 255.0f * 0.9f, g / 255.0f * 0.9f, b / 255.0f * 0.9f);
    drawPyramid(x, y, z, size * s * 0.7f, size * s, size * s * 0.7f, r, g, b);
    setEmission(0.0f, 0.0f, 0.0f);
}

static void drawCampfire(float x, float z)
{
    drawCube(x + 0.8f, 0.0f, z, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x - 0.8f, 0.0f, z, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x, 0.0f, z + 0.8f, 0.4f, 0.3f, 0.4f, 120, 120, 125);
    drawCube(x, 0.0f, z - 0.8f, 0.4f, 0.3f, 0.4f, 120, 120, 125);

    drawCube(x, 0.0f, z, 1.2f, 0.25f, 0.25f, 90, 55, 30);                 // log

    drawFlame(x, 0.25f, z, 1.0f, 6.0f, 0.0f, 255, 120, 30);
    drawFlame(x + 0.25f, 0.25f, z, 0.6f, 8.0f, 2.0f, 230, 70, 20);
    drawFlame(x - 0.2f, 0.25f, z, 0.5f, 10.0f, 4.0f, 255, 210, 60);
}

// A-frame tent; angle turns the tent so the door faces the campfire
static void drawTent(float x, float z, float angle, int r, int g, int b)
{
    float width = 3.6f, height = 2.6f, length = 4.6f;
    glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        drawPrism(0.0f, 0.0f, 0.0f, width, height, length, r, g, b);                       // main tent
        drawPrism(0.0f, 0.0f, length / 2 + 0.02f, width * 0.45f, height * 0.65f, 0.05f,
                  185, 140, 80);                                                           // door
    glPopMatrix();
}

static void drawPineTree(float x, float z, float s)
{
    drawCube(x, 0.0f, z, 0.4f * s, 1.3f * s, 0.4f * s, 85, 55, 30);
    drawPyramid(x, 1.0f * s, z, 2.6f * s, 2.0f * s, 2.6f * s, 30, 90, 50);
    drawPyramid(x, 2.4f * s, z, 2.0f * s, 1.8f * s, 2.0f * s, 36, 105, 58);
}

static void drawRock(float x, float z, float size)
{
    drawPyramid(x, 0.0f, z, size * 1.4f, size, size * 1.2f, 110, 110, 115);
}

static void drawLantern(float x, float z)
{
    drawCube(x, 0.0f, z, 0.1f, 1.6f, 0.1f, 70, 45, 25);                   // post
    setEmission(0.9f, 0.7f, 0.3f);                                        // the lamp itself glows
    drawCube(x, 1.6f, z, 0.3f, 0.4f, 0.3f, 255, 215, 100);
    setEmission(0.0f, 0.0f, 0.0f);
}

static void drawEntranceSign(float x, float z)
{
    drawCube(x - 1.8f, 0.0f, z, 0.25f, 2.6f, 0.25f, 95, 62, 35);
    drawCube(x + 1.8f, 0.0f, z, 0.25f, 2.6f, 0.25f, 95, 62, 35);
    drawCube(x, 1.5f, z, 3.8f, 0.9f, 0.15f, 140, 95, 55);
}

static void drawTrees()
{
    for (int i = 0; i < treeCount; i++)
        drawPineTree(treeX[i], treeZ[i], treeScale[i]);
}

static void drawRocks()
{
    drawRock(-15.0f, -8.0f, 1.0f);
    drawRock(-15.0f, 4.0f, 1.3f);
    drawRock(15.0f, -9.0f, 1.1f);
    drawRock(15.0f, 2.0f, 1.4f);
    drawRock(-10.0f, -18.0f, 1.2f);
    drawRock(11.0f, -18.0f, 1.0f);
}

static void drawLanterns()
{
    drawLantern(-4.5f, -3.0f);
    drawLantern(4.5f, -3.0f);
    drawLantern(-2.0f, -6.0f);
    drawLantern(2.0f, -6.0f);
    drawLantern(-3.0f, 15.0f);
    drawLantern(3.0f, 15.0f);
}

// Whole campsite. Lighting and fog are ON here, so the campfire, the moon fill,
// the meteors and the flash all affect these objects.
static void drawCampsite()
{
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);

    drawGround();
    drawPlane();
    drawCliff();

    drawCampfire(0.0f, -5.0f);

    // the angles turn the doors toward the campfire
    drawTent(-8.0f, -3.0f, 104.0f, 253, 208, 120);
    drawTent(8.0f, -3.0f, -104.0f, 245, 185, 100);

    drawEntranceSign(0.0f, 15.0f);

    drawLanterns();
    drawTrees();
    drawRocks();

    glDisable(GL_LIGHTING);
}

// ============================================================================
//  GLUT CALLBACKS
// ============================================================================

// ----------------------------------------------------------------------------
//  [MODIFIED] display: the layer order (back to front)
//    1  night sky gradient + far mountains          (no depth writes)
//    2  stars, haze layers, distant clouds          (no depth writes)
//    3  fast meteors, panel clouds                  (no depth writes)
//    4  campsite ground + objects                   (depth test + depth writes, fog)
//    5  main meteor tail, derailed trail, glow heads, explosion
//                                                   (depth TEST on, depth WRITES off)
//    6  full-screen white flash                     (foreground, 2D)
//  The sky is drawn first with depth writes off, so the campsite can never be hidden
//  by clouds. The glow pass is depth tested, so trees and tents correctly cover the
//  meteors, but the meteors still shine over the sky and the ground.
// ----------------------------------------------------------------------------
static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camX, camY, camZ,
              camX + frontX, camY + frontY, camZ + frontZ,
              0.0f, 1.0f, 0.0f);

    updateMeteorLighting();                        // lights in world coordinates (after gluLookAt)

    // ---- sky pass ----
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_FOG);
    drawNightSky();
    drawStars();
    drawAtmosphere();
    drawClouds();                                  // distant clouds all around
    drawAdditionalMeteors();
    drawPanelClouds();

    // ---- campsite pass ----
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_FOG);
    drawCampsite();
    glDisable(GL_FOG);

    // ---- glow pass (depth test on, depth writes off) ----
    glDepthMask(GL_FALSE);
    drawMainMeteor();
    drawDerailedMeteor();
    drawMeteorGlows();

    // ---- foreground ----
    drawWhiteFlash();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h < 1) h = 1;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 0.1, 900.0);   // far plane covers the sky dome and the long meteor tail
    glMatrixMode(GL_MODELVIEW);
}

static void toggleFullscreen()
{
    if (!g_fullscreen)
    {
        g_savedX = glutGet(GLUT_WINDOW_X);
        g_savedY = glutGet(GLUT_WINDOW_Y);
        g_savedW = glutGet(GLUT_WINDOW_WIDTH);
        g_savedH = glutGet(GLUT_WINDOW_HEIGHT);
        glutFullScreen();
        g_fullscreen = true;
    }
    else
    {
        glutReshapeWindow(g_savedW, g_savedH);
        glutPositionWindow(g_savedX, g_savedY);
        g_fullscreen = false;
    }
    firstMouse = true;
}

// Mouse look (from the campsite program). Used for button-up and button-down movement.
static void mouseMovement(int x, int y)
{
    if (!mouseLook) return;

    int centerX = glutGet(GLUT_WINDOW_WIDTH) / 2;
    int centerY = glutGet(GLUT_WINDOW_HEIGHT) / 2;

    if (firstMouse)
    {
        glutWarpPointer(centerX, centerY);
        firstMouse = false;
        return;
    }

    float offsetX = (float)(x - centerX);
    float offsetY = (float)(centerY - y);

    yaw   += offsetX * sensitivity;
    pitch += offsetY * sensitivity;
    pitch  = clampf(pitch, -89.0f, 89.0f);
    updateCameraDirection();

    glutWarpPointer(centerX, centerY);
}

// Timeline scrubbing: the wheel only ADDS seconds to g_scrubPending; updateGlobalSceneTime()
// applies them smoothly while the clock keeps running. Nothing blocks, nothing pauses.
static void scrubTimeline(int direction)
{
    g_scrubPending += (direction > 0 ? WHEEL_STEP : -WHEEL_STEP);
}

// FreeGLUT wheel callback: +1 = wheel up (forward in time), -1 = wheel down (rewind)
static void mouseWheel(int wheel, int direction, int x, int y)
{
    (void)wheel; (void)x; (void)y;
    scrubTimeline(direction);
}

// Fallback for GLUT versions that report the wheel as buttons 3 (up) and 4 (down)
static void mouse(int button, int state, int x, int y)
{
    (void)x; (void)y;
    if (state == GLUT_DOWN)
    {
        if (button == 3) scrubTimeline(+1);
        else if (button == 4) scrubTimeline(-1);
    }
}

static void specialKeys(int key, int x, int y)
{
    (void)x; (void)y;
    if (key == GLUT_KEY_LEFT)  leftArrow  = true;
    if (key == GLUT_KEY_RIGHT) rightArrow = true;
    if (key == GLUT_KEY_UP)    upArrow    = true;
    if (key == GLUT_KEY_DOWN)  downArrow  = true;
}

static void specialKeysUp(int key, int x, int y)
{
    (void)x; (void)y;
    if (key == GLUT_KEY_LEFT)  leftArrow  = false;
    if (key == GLUT_KEY_RIGHT) rightArrow = false;
    if (key == GLUT_KEY_UP)    upArrow    = false;
    if (key == GLUT_KEY_DOWN)  downArrow  = false;
}

static void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
        case 'w': case 'W': wPressed = true; break;
        case 's': case 'S': sPressed = true; break;
        case 'a': case 'A': aPressed = true; break;
        case 'd': case 'D': dPressed = true; break;

        case ' ':           g_paused = !g_paused; break;         // pause / resume the meteor clock
        case 'r': case 'R': sceneTime = 0.0f; g_scrubPending = 0.0f; break;   // restart the timeline
        case 'c': case 'C':                                       // camera back to the spawn point
            camX = 0.0f; camZ = 0.0f; yaw = -90.0f; pitch = 0.0f;
            updateCameraDirection();
            break;
        case 'f': case 'F': toggleFullscreen(); break;
        case 'm': case 'M':                                       // release / capture the mouse
            mouseLook = !mouseLook;
            firstMouse = true;
            glutSetCursor(mouseLook ? GLUT_CURSOR_NONE : GLUT_CURSOR_LEFT_ARROW);
            break;
        case '[': scrubTimeline(-1); break;                       // timeline fallback keys
        case ']': scrubTimeline(+1); break;
        case 27:  std::exit(0); break;                            // ESC
    }
}

static void keyboardUp(unsigned char key, int, int)
{
    switch (key)
    {
        case 'w': case 'W': wPressed = false; break;
        case 's': case 'S': sPressed = false; break;
        case 'a': case 'A': aPressed = false; break;
        case 'd': case 'D': dPressed = false; break;
    }
}

// ----------------------------------------------------------------------------
//  timer: ONE place that advances everything, about 60 times per second.
//  Order: global time -> meteor animation -> camera. The camera is the last
//  step and has no influence on the first two.
// ----------------------------------------------------------------------------
static void timer(int)
{
    static int lastMs = -1;
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if (lastMs < 0) lastMs = nowMs;
    float dt = (nowMs - lastMs) / 1000.0f;
    lastMs = nowMs;
    if (dt > 0.1f) dt = 0.1f;

    fireTime = nowMs / 1000.0f;                    // campfire clock (real time)

    updateGlobalSceneTime(dt);
    updateMeteorAnimation();
    updateCamera(dt);

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Night Campsite + Meteor Sky  (look up!  wheel = timeline, F = fullscreen)");

    initializeOpenGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutMouseFunc(mouse);                          // wheel fallback (buttons 3 / 4)
#ifdef FREEGLUT
    glutMouseWheelFunc(mouseWheel);                // real wheel callback (FreeGLUT)
#endif
    glutPassiveMotionFunc(mouseMovement);          // mouse look
    glutMotionFunc(mouseMovement);                 // ... also while a button is held
    glutTimerFunc(16, timer, 0);

    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(640, 360);
    toggleFullscreen();                            // open in fullscreen automatically

    glutMainLoop();
    return 0;
}