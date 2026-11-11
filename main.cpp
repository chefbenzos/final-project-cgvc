// ============================================================================
//  Night Sky with Glowing Meteors  (inspired by the comet scene of "Your Name")
//  Introductory OpenGL course project
//
//  Uses ONLY: C++, GLUT/FreeGLUT and the traditional fixed-function pipeline
//  (glBegin/glEnd, vertex arrays, one VBO, matrix stack, fixed-function
//  lighting, alpha blending and a few textures generated in code).
//  NO shaders, NO GLSL, NO GLFW/GLAD/GLEW/SDL, NO image files.
//
//  Build (Linux):    g++ main.cpp -o meteor -lglut -lGLU -lGL
//  Build (MinGW):    g++ main.cpp -o meteor.exe -lfreeglut -lglu32 -lopengl32
//  Build (macOS):    g++ main.cpp -o meteor -framework GLUT -framework OpenGL
//
//  Keys:  SPACE = pause / resume     R = reset     ESC = quit
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
#include <GL/freeglut.h>
#endif

// ----------------------------------------------------------------------------
//  Scene constants
// ----------------------------------------------------------------------------
static const float WORLD_W = 1280.0f;          // our 2D "world" is 1280 x 720 (16:9)
static const float WORLD_H = 720.0f;
static const float PI      = 3.14159265f;

// Main comet: head sits at the reference pose, tail extends toward the upper-right.
static const float MAIN_ANGLE_DEG = 36.0f;     // direction of the tail (from the head)
static const float MAIN_REF_X     = 435.0f;    // head position that matches the reference image
static const float MAIN_REF_Y     = 225.0f;
static const float MAIN_SPEED     = -150.0f;   // pixels / second along the tail axis.
                                               // NEGATIVE = head travels down-left (like the film).
                                               // Use +150 if you want it to travel up-right instead.
static const float TAIL_LEN       = 1900.0f;
static const float TAIL_CURVE     = 3.0e-5f;   // how much the tail bends
static const float MAIN_MAX_HW    = 140.0f;    // widest half-width of the tail layers

// Small meteor
static const float SMALL_ANGLE_DEG = 53.0f;
static const float SMALL_REF_X     = 545.0f;
static const float SMALL_REF_Y     = 172.0f;
static const float SMALL_SPEED     = -110.0f;
static const float SMALL_LEN       = 260.0f;

// ----------------------------------------------------------------------------
//  Tiny helpers
// ----------------------------------------------------------------------------
static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static float smoothstep(float a, float b, float x)
{
    float t = clampf((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
static float frand01() { return (float)rand() / (float)RAND_MAX; }
static float frand(float a, float b) { return a + (b - a) * frand01(); }
static float deg2rad(float d) { return d * PI / 180.0f; }

// ----------------------------------------------------------------------------
//  VBO access (VBOs are OpenGL 1.5, so on Windows/Linux we fetch them through
//  FreeGLUT's glutGetProcAddress instead of using an extension library).
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

// ----------------------------------------------------------------------------
//  Data structures
// ----------------------------------------------------------------------------
struct Vtx   { float x, y, r, g, b, a; };                 // interleaved position + colour
struct Range { GLint first; GLsizei count; GLenum mode; }; // one glDrawArrays call

static Vtx makeVtx(float x, float y, float r, float g, float b, float a)
{
    Vtx v; v.x = x; v.y = y; v.r = r; v.g = g; v.b = b; v.a = a; return v;
}
static Range makeRange(GLint first, GLsizei count, GLenum mode)
{
    Range r; r.first = first; r.count = count; r.mode = mode; return r;
}

// Stars (three groups, one per point size, each stored in vertex arrays)
struct StarSet
{
    float size;
    std::vector<float> pos;      // x,y
    std::vector<float> col;      // r,g,b,a  (alpha changes when twinkling)
    std::vector<float> base;     // base brightness
    std::vector<float> phase;
    std::vector<float> speed;
    std::vector<char>  twinkle;
};
static StarSet g_stars[3];

// Clouds
struct Puff  { float ox, oy, r, rot, shade; };
struct Cloud { float x, y, phase; std::vector<Puff> puffs; };
static std::vector<Cloud> g_clouds;

// Main comet geometry -> stored in a VBO
static std::vector<Vtx>   g_meteorVerts;
static std::vector<Range> g_meteorRanges;
static GLuint             g_meteorVBO = 0;

// Small meteor trail -> plain client-side vertex array
static std::vector<Vtx>   g_smallVerts;
static std::vector<Range> g_smallRanges;

// Debris sparkles near the comet head (vertex arrays, world-axis aligned)
static const int DEBRIS_N = 22;
static float g_debrisPos[DEBRIS_N * 2];
static float g_debrisCol[DEBRIS_N * 4];

// Textures
static GLuint g_glowTex  = 0;
static GLuint g_cloudTex = 0;

// Animation state
static bool  g_paused      = false;
static float g_time        = 0.0f;   // animation clock (stops while paused)
static float g_mainS       = 0.0f;   // comet head distance along its axis from the reference pose
static float g_smallS      = 0.0f;
static bool  g_mainFading  = false;
static float g_mainFade    = 0.0f;   // 0 = fully visible, 1 = hidden
static const float FADE_TIME = 1.2f;

// ----------------------------------------------------------------------------
//  Forward declarations (required program structure)
// ----------------------------------------------------------------------------
static void initializeOpenGL();
static void setupLighting();
static void setupTextures();
static void createStars();
static void createClouds();
static void createMeteorGeometry();
static void drawStars();
static void drawClouds();
static void drawMainMeteor();
static void drawSmallMeteor();
static void updateAnimation();
static void display();
static void reshape(int w, int h);
static void keyboard(unsigned char key, int x, int y);

// ----------------------------------------------------------------------------
//  Small drawing helpers
// ----------------------------------------------------------------------------

// A square [-s, s] with texture coordinates (used for glows and cloud puffs).
static void drawTexturedQuad(float s, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glNormal3f(0.0f, 0.0f, 1.0f);              // only matters when lighting is on
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-s,  s);
    glEnd();
}

// Full-screen night-sky gradient. With alpha < 1 it is also used to fade things out.
static void drawBackground(float alpha)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
        glColor4f(0.070f, 0.110f, 0.240f, alpha);   // bottom (a bit lighter navy)
        glVertex2f(0.0f, 0.0f);
        glVertex2f(WORLD_W, 0.0f);
        glColor4f(0.035f, 0.055f, 0.150f, alpha);   // top (darker navy)
        glVertex2f(WORLD_W, WORLD_H);
        glVertex2f(0.0f, WORLD_H);
    glEnd();
}

static void mainHeadPos(float &x, float &y)
{
    float a = deg2rad(MAIN_ANGLE_DEG);
    x = MAIN_REF_X + cosf(a) * g_mainS;
    y = MAIN_REF_Y + sinf(a) * g_mainS;
}
static void smallHeadPos(float &x, float &y)
{
    float a = deg2rad(SMALL_ANGLE_DEG);
    x = SMALL_REF_X + cosf(a) * g_smallS;
    y = SMALL_REF_Y + sinf(a) * g_smallS;
}

static float mainPulse()  { return 1.0f + 0.12f * sinf(g_time * 6.0f) + 0.04f * sinf(g_time * 15.0f); }
static float smallPulse() { return 1.0f + 0.15f * sinf(g_time * 8.0f + 1.0f); }

// ----------------------------------------------------------------------------
//  Procedural noise (used only to make the cloud texture edges irregular)
// ----------------------------------------------------------------------------
static float hash2(int x, int y)
{
    unsigned n = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= (n >> 16);
    return (float)(n & 0xFFFFu) / 65535.0f;
}
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

// ----------------------------------------------------------------------------
//  initializeOpenGL
// ----------------------------------------------------------------------------
static void initializeOpenGL()
{
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
    glDisable(GL_DEPTH_TEST);                      // pure 2D layering, painter's algorithm
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glShadeModel(GL_SMOOTH);

    g_useVBO = loadVBOFunctions();
    if (!g_useVBO)
        std::printf("VBO functions not available - using ordinary vertex arrays instead.\n");

    srand(7);                                      // same random scene every run
    setupLighting();
    setupTextures();
    createStars();
    createClouds();
    createMeteorGeometry();
}

// ----------------------------------------------------------------------------
//  setupLighting  (fixed-function lights, used on the clouds)
// ----------------------------------------------------------------------------
static void setupLighting()
{
    // A little cool blue light that fills the whole scene
    GLfloat globalAmbient[] = { 0.28f, 0.34f, 0.50f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // LIGHT0 = glow of the comet head, LIGHT1 = glow of the tail
    GLfloat black[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat white[]  = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT,  black);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, black);
    glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION,   0.0004f);

    glLightfv(GL_LIGHT1, GL_AMBIENT,  black);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  white);
    glLightfv(GL_LIGHT1, GL_SPECULAR, black);
    glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,   0.0006f);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    // glColor3f/4f will set the material, so every puff can have its own colour
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);                        // keep normals unit length after glScalef
    // NOTE: GL_LIGHTING itself is switched on only while the clouds are drawn.
}

// ----------------------------------------------------------------------------
//  setupTextures  (both textures are created in code, no image files)
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

    // ---- 1) soft round glow: white, alpha fades smoothly from the centre ----
    {
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

    // ---- 2) cloud puff: round blob with a noisy, slightly crisp edge ----
    {
        const int N = 128;
        std::vector<unsigned char> px(N * N * 4);
        for (int j = 0; j < N; ++j)
            for (int i = 0; i < N; ++i)
            {
                float u = ((i + 0.5f) / N) * 2.0f - 1.0f;
                float v = ((j + 0.5f) / N) * 2.0f - 1.0f;
                float d = sqrtf(u * u + v * v);
                float n = valueNoise(u * 3.0f + 5.0f, v * 3.0f + 5.0f)
                        + 0.5f * valueNoise(u * 7.0f + 11.0f, v * 7.0f + 11.0f);   // 0 .. 1.5
                float edge = 0.70f + 0.28f * (n / 1.5f);
                float a = clampf((edge - d) / 0.30f, 0.0f, 1.0f);
                a = a * a * (3.0f - 2.0f * a);
                float shade = 0.80f + 0.20f * (v * 0.5f + 0.5f);                   // lighter on top
                unsigned char *p = &px[(j * N + i) * 4];
                p[0] = p[1] = p[2] = (unsigned char)(shade * 255.0f);
                p[3] = (unsigned char)(a * 255.0f);
            }
        g_cloudTex = makeTexture(px, N);
    }
}

// ----------------------------------------------------------------------------
//  createStars  (vertex arrays: positions + colours, three point sizes)
// ----------------------------------------------------------------------------
static void createStars()
{
    const float sizes[3]  = { 1.3f, 2.2f, 3.4f };
    const int   counts[3] = { 420, 130, 26 };

    for (int s = 0; s < 3; ++s)
    {
        StarSet &set = g_stars[s];
        set.size = sizes[s];
        for (int i = 0; i < counts[s]; ++i)
        {
            float b = (s == 2) ? frand(0.8f, 1.0f) : frand(0.30f, 1.0f);
            float tint = frand01();
            float r, g, bl;
            if (tint < 0.55f)      { r = 0.80f; g = 0.88f; bl = 1.00f; }   // blue-white
            else if (tint < 0.85f) { r = 1.00f; g = 1.00f; bl = 1.00f; }   // white
            else                   { r = 1.00f; g = 0.92f; bl = 0.78f; }   // warm

            set.pos.push_back(frand(0.0f, WORLD_W));
            set.pos.push_back(frand(0.0f, WORLD_H));
            set.col.push_back(r); set.col.push_back(g); set.col.push_back(bl); set.col.push_back(b);
            set.base.push_back(b);
            set.phase.push_back(frand(0.0f, 2.0f * PI));
            set.speed.push_back(frand(1.5f, 4.5f));
            set.twinkle.push_back(frand01() < (s == 2 ? 0.85f : 0.40f) ? 1 : 0);
        }
    }
}

static void updateStars()
{
    for (int s = 0; s < 3; ++s)
    {
        StarSet &set = g_stars[s];
        for (size_t i = 0; i < set.base.size(); ++i)
            if (set.twinkle[i])
                set.col[i * 4 + 3] = set.base[i] * (0.55f + 0.45f * sinf(g_time * set.speed[i] + set.phase[i]));
    }
}

// ----------------------------------------------------------------------------
//  createClouds  (each cloud = many overlapping textured puffs)
// ----------------------------------------------------------------------------
static void addCloud(float cx, float cy, float w, float h, int count)
{
    Cloud c;
    c.x = cx; c.y = cy; c.phase = frand(0.0f, 6.28f);
    for (int i = 0; i < count; ++i)
    {
        float ang = frand(0.0f, 2.0f * PI);
        float rad = sqrtf(frand01());
        Puff p;
        p.ox    = cosf(ang) * rad * w * 0.5f;
        p.oy    = sinf(ang) * rad * h * 0.5f - 0.08f * h;         // flatter, heavier bottom
        p.r     = h * frand(0.20f, 0.42f);
        p.rot   = frand(0.0f, 360.0f);
        p.shade = frand(0.88f, 1.10f);
        c.puffs.push_back(p);
    }
    g_clouds.push_back(c);
}

static void createClouds()
{
    //        x      y     width height puffs
    addCloud( 720.0f, 648.0f, 230.0f,  90.0f, 14);   // top centre
    addCloud( 527.0f, 473.0f, 125.0f,  70.0f,  9);   // small, left of centre
    addCloud(1054.0f, 535.0f, 280.0f, 200.0f, 26);   // big, right
    addCloud( 892.0f, 450.0f, 140.0f,  75.0f, 10);
    addCloud( 828.0f, 245.0f, 250.0f, 160.0f, 22);   // lower right
    addCloud( 997.0f, 391.0f,  55.0f,  40.0f,  4);   // tiny
}

// ----------------------------------------------------------------------------
//  createMeteorGeometry
//    Everything is built in "local" coordinates: head at (0,0), tail along +x.
//    The comet is later rotated/translated with matrices.
// ----------------------------------------------------------------------------
static void pushStrips(std::vector<Vtx> &verts, std::vector<Range> &ranges,
                       const std::vector<Vtx> &top, const std::vector<Vtx> &mid,
                       const std::vector<Vtx> &bot)
{
    // Upper half: top edge (alpha 0) -> centre (bright)
    GLint first = (GLint)verts.size();
    for (size_t i = 0; i < top.size(); ++i) { verts.push_back(top[i]); verts.push_back(mid[i]); }
    ranges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));

    // Lower half: centre (bright) -> bottom edge (alpha 0)
    first = (GLint)verts.size();
    for (size_t i = 0; i < top.size(); ++i) { verts.push_back(mid[i]); verts.push_back(bot[i]); }
    ranges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));
}

struct LayerDef { float r, g, b, a, w, off; };

static void addMainLayer(const LayerDef &d)
{
    const int N = 90;
    std::vector<Vtx> top, mid, bot;
    for (int i = 0; i <= N; ++i)
    {
        float t    = (float)i / (float)N;
        float x    = t * TAIL_LEN;
        float prof = 0.012f + 0.988f * powf(t, 1.2f);          // thin at the head, wide far away
        float cy   = -TAIL_CURVE * x * x;                      // gentle curve of the tail
        float hw   = MAIN_MAX_HW * d.w * prof;
        float yc   = cy + d.off * MAIN_MAX_HW * prof;          // layer offset across the tail
        float a    = d.a * (0.25f + 0.75f * smoothstep(0.0f, 0.35f, t))
                         * (1.0f - smoothstep(0.88f, 1.0f, t));
        top.push_back(makeVtx(x, yc + hw, d.r, d.g, d.b, 0.0f));
        mid.push_back(makeVtx(x, yc,      d.r, d.g, d.b, a));
        bot.push_back(makeVtx(x, yc - hw, d.r, d.g, d.b, 0.0f));
    }
    pushStrips(g_meteorVerts, g_meteorRanges, top, mid, bot);
}

static void createMeteorGeometry()
{
    // ---------- main comet: coloured layers (outer glow first) ----------
    const LayerDef layers[] = {
        { 0.10f, 0.30f, 1.00f, 0.16f, 2.00f,  0.00f },   // wide transparent blue glow
        { 0.15f, 0.40f, 1.00f, 0.45f, 1.05f,  0.20f },   // blue body
        { 0.45f, 0.75f, 1.00f, 0.50f, 0.35f,  0.60f },   // light blue
        { 0.10f, 0.95f, 1.00f, 0.55f, 0.30f,  0.10f },   // cyan
        { 0.60f, 0.28f, 1.00f, 0.60f, 0.40f, -0.45f },   // purple
        { 1.00f, 0.25f, 0.75f, 0.65f, 0.22f, -0.82f },   // magenta edge
        { 0.85f, 0.95f, 1.00f, 0.95f, 0.09f,  0.85f }    // white-blue core
    };
    for (size_t i = 0; i < sizeof(layers) / sizeof(layers[0]); ++i)
        addMainLayer(layers[i]);

    // ---------- thin streak lines beside the head ----------
    {
        const float streaks[][5] = {            // x0, y0, x1, y1, alpha
            { 140.0f,  18.0f,  760.0f,  70.0f, 0.70f },
            { 260.0f,  30.0f,  900.0f, 120.0f, 0.60f },
            {  80.0f,   6.0f,  520.0f,  38.0f, 0.55f },
            { 380.0f,  40.0f, 1100.0f, 170.0f, 0.50f },
            { 120.0f, -10.0f,  600.0f, -60.0f, 0.40f },
            { 200.0f,  55.0f,  640.0f, 150.0f, 0.45f }
        };
        GLint first = (GLint)g_meteorVerts.size();
        for (int i = 0; i < 6; ++i)
        {
            g_meteorVerts.push_back(makeVtx(streaks[i][0], streaks[i][1], 0.55f, 0.80f, 1.0f, streaks[i][4]));
            g_meteorVerts.push_back(makeVtx(streaks[i][2], streaks[i][3], 0.55f, 0.80f, 1.0f, 0.0f));
        }
        g_meteorRanges.push_back(makeRange(first, 12, GL_LINES));
    }

    // ---------- upload to a Vertex Buffer Object ----------
    if (g_useVBO)
    {
        vboGen(&g_meteorVBO);
        vboBind(g_meteorVBO);
        vboData((VboSize)(g_meteorVerts.size() * sizeof(Vtx)), &g_meteorVerts[0]);
        vboBind(0);
    }

    // ---------- small meteor trail (thin, white-blue, tapering) ----------
    {
        const int N = 30;
        std::vector<Vtx> top, mid, bot;
        for (int i = 0; i <= N; ++i)
        {
            float t  = (float)i / (float)N;
            float x  = t * SMALL_LEN;
            float hw = 0.4f + 2.4f * powf(1.0f - t, 0.8f);
            float a  = 0.95f * powf(1.0f - t, 1.2f);
            top.push_back(makeVtx(x,  hw, 0.70f, 0.85f, 1.0f, 0.0f));
            mid.push_back(makeVtx(x, 0.0f, 0.85f, 0.93f, 1.0f, a));
            bot.push_back(makeVtx(x, -hw, 0.70f, 0.85f, 1.0f, 0.0f));
        }
        pushStrips(g_smallVerts, g_smallRanges, top, mid, bot);
    }

    // ---------- debris sparkles around the comet head ----------
    for (int i = 0; i < DEBRIS_N; ++i)
    {
        g_debrisPos[i * 2 + 0] = frand(5.0f, 125.0f);
        g_debrisPos[i * 2 + 1] = -frand(3.0f, 68.0f);
        bool pink = frand01() < 0.3f;
        g_debrisCol[i * 4 + 0] = pink ? 1.0f : 0.5f;
        g_debrisCol[i * 4 + 1] = pink ? 0.5f : 0.85f;
        g_debrisCol[i * 4 + 2] = pink ? 0.7f : 1.0f;
        g_debrisCol[i * 4 + 3] = frand(0.35f, 0.9f);
    }
}

// ----------------------------------------------------------------------------
//  drawStars
// ----------------------------------------------------------------------------
static void drawStars()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    for (int s = 0; s < 3; ++s)
    {
        StarSet &set = g_stars[s];
        glPointSize(set.size);
        glVertexPointer(2, GL_FLOAT, 0, &set.pos[0]);
        glColorPointer(4, GL_FLOAT, 0, &set.col[0]);
        glDrawArrays(GL_POINTS, 0, (GLsizei)set.base.size());
    }
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// ----------------------------------------------------------------------------
//  drawClouds  (lit + textured + alpha blended)
// ----------------------------------------------------------------------------
static void drawClouds()
{
    // --- place the two lights (positions are transformed by the current modelview) ---
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float hx, hy;
    mainHeadPos(hx, hy);
    float ang = deg2rad(MAIN_ANGLE_DEG);
    float k = (1.0f - g_mainFade) * mainPulse();               // comet light pulses with the head

    GLfloat pos0[]  = { hx, hy, 260.0f, 1.0f };                // light at the comet head
    GLfloat pos1[]  = { hx + cosf(ang) * 900.0f, hy + sinf(ang) * 900.0f, 300.0f, 1.0f };  // on the tail
    GLfloat dif0[]  = { 1.30f * k, 1.60f * k, 2.30f * k, 1.0f };
    GLfloat dif1[]  = { 1.00f * k, 0.55f * k, 1.50f * k, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif0);
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  dif1);

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_cloudTex);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);          // normal transparency

    for (size_t c = 0; c < g_clouds.size(); ++c)
    {
        const Cloud &cl = g_clouds[c];
        float dx = sinf(g_time * 0.12f + cl.phase) * 30.0f;     // slow drift
        float dy = sinf(g_time * 0.09f + cl.phase * 1.7f) * 8.0f;

        glPushMatrix();
        glTranslatef(cl.x + dx, cl.y + dy, 0.0f);

        for (int pass = 0; pass < 2; ++pass)                    // pass 0 = dark body, pass 1 = lit highlights
        {
            for (size_t i = 0; i < cl.puffs.size(); ++i)
            {
                const Puff &p = cl.puffs[i];
                glPushMatrix();
                if (pass == 0)
                {
                    glTranslatef(p.ox, p.oy, 0.0f);
                    glRotatef(p.rot, 0.0f, 0.0f, 1.0f);
                    glScalef(p.r * 1.15f, p.r * 0.85f, 1.0f);
                    drawTexturedQuad(1.0f, 0.36f * p.shade, 0.46f * p.shade, 0.66f * p.shade, 0.82f);
                }
                else
                {
                    glTranslatef(p.ox - 0.25f * p.r, p.oy - 0.20f * p.r, 0.0f);  // toward the comet
                    glRotatef(p.rot, 0.0f, 0.0f, 1.0f);
                    glScalef(p.r * 0.70f, p.r * 0.55f, 1.0f);
                    drawTexturedQuad(1.0f, 0.70f, 0.80f, 0.98f, 0.50f);
                }
                glPopMatrix();
            }
        }
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------------------------
//  drawMainMeteor
// ----------------------------------------------------------------------------
static void drawMainMeteor()
{
    float hx, hy;
    mainHeadPos(hx, hy);
    float p = mainPulse();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);                          // additive glow

    glPushMatrix();
    glTranslatef(hx, hy, 0.0f);                                 // move to the head

    // ---- debris sparkles (not rotated) ----
    glPointSize(2.2f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, g_debrisPos);
    glColorPointer(4, GL_FLOAT, 0, g_debrisCol);
    glDrawArrays(GL_POINTS, 0, DEBRIS_N);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glRotatef(MAIN_ANGLE_DEG, 0.0f, 0.0f, 1.0f);                // point local +x along the tail

    // ---- the tail layers, straight from the VBO ----
    const char *base = 0;
    if (g_useVBO) vboBind(g_meteorVBO);
    else          base = (const char *)&g_meteorVerts[0];

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(Vtx), base);
    glColorPointer(4, GL_FLOAT, sizeof(Vtx), base + 2 * sizeof(float));

    glLineWidth(1.5f);
    for (size_t i = 0; i < g_meteorRanges.size(); ++i)
        glDrawArrays(g_meteorRanges[i].mode, g_meteorRanges[i].first, g_meteorRanges[i].count);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    if (g_useVBO) vboBind(0);

    // ---- the glowing head (several textured, additive quads) ----
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_glowTex);
    drawTexturedQuad(110.0f * p, 0.15f, 0.35f, 1.00f, 0.35f);   // big blue halo
    drawTexturedQuad( 55.0f * p, 0.30f, 0.70f, 1.00f, 0.60f);   // cyan glow
    drawTexturedQuad( 24.0f * p, 0.70f, 0.95f, 1.00f, 0.90f);   // bright inner glow
    drawTexturedQuad(  9.0f * p, 1.00f, 1.00f, 1.00f, 1.00f);   // white core

    glPushMatrix();                                             // flare along the tail axis
    glScalef(3.0f, 0.12f, 1.0f);
    drawTexturedQuad(40.0f * p, 0.60f, 0.85f, 1.0f, 0.70f);
    glPopMatrix();
    glPushMatrix();                                             // short flare across the tail
    glScalef(0.12f, 2.0f, 1.0f);
    drawTexturedQuad(25.0f * p, 0.60f, 0.85f, 1.0f, 0.45f);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// ----------------------------------------------------------------------------
//  drawSmallMeteor
// ----------------------------------------------------------------------------
static void drawSmallMeteor()
{
    float hx, hy;
    smallHeadPos(hx, hy);
    float p = smallPulse();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(hx, hy, 0.0f);
    glRotatef(SMALL_ANGLE_DEG, 0.0f, 0.0f, 1.0f);

    // thin blue-white trail (ordinary client-side vertex array)
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    const char *base = (const char *)&g_smallVerts[0];
    glVertexPointer(2, GL_FLOAT, sizeof(Vtx), base);
    glColorPointer(4, GL_FLOAT, sizeof(Vtx), base + 2 * sizeof(float));
    for (size_t i = 0; i < g_smallRanges.size(); ++i)
        glDrawArrays(g_smallRanges[i].mode, g_smallRanges[i].first, g_smallRanges[i].count);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    // pink / white glowing head
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_glowTex);
    drawTexturedQuad(30.0f * p, 1.00f, 0.35f, 0.55f, 0.45f);    // pink halo
    drawTexturedQuad(13.0f * p, 1.00f, 0.60f, 0.75f, 0.85f);
    drawTexturedQuad( 6.0f * p, 1.00f, 0.95f, 0.97f, 1.00f);    // white core
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

// ----------------------------------------------------------------------------
//  updateAnimation  (delta-time based)
// ----------------------------------------------------------------------------
static void resetAnimation()
{
    g_time       = 0.0f;
    g_mainS      = 0.0f;                 // back to the reference-image pose
    g_smallS     = 0.0f;
    g_mainFading = false;
    g_mainFade   = 0.0f;
    updateStars();
}

static void updateAnimation()
{
    static int lastMs = -1;
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if (lastMs < 0) lastMs = nowMs;
    float dt = (nowMs - lastMs) / 1000.0f;
    lastMs = nowMs;
    if (dt > 0.1f) dt = 0.1f;            // avoid huge jumps (window drag, etc.)
    if (g_paused) return;

    g_time += dt;

    // ----- main comet -----
    g_mainS += MAIN_SPEED * dt;
    bool mainOut = (MAIN_SPEED < 0.0f) ? (g_mainS < -520.0f) : (g_mainS > 1100.0f);
    if (mainOut && !g_mainFading)
    {
        g_mainFading = true;
        g_mainFade   = 0.0f;
    }
    if (g_mainFading)
    {
        g_mainFade += dt / FADE_TIME;
        if (g_mainFade >= 1.0f)          // fully faded out: restart from the far side
        {
            g_mainFading = false;
            g_mainFade   = 0.0f;
            g_mainS      = (MAIN_SPEED < 0.0f) ? 1100.0f : -700.0f;
        }
    }

    // ----- small meteor (independent) -----
    g_smallS += SMALL_SPEED * dt;
    if (SMALL_SPEED < 0.0f) { if (g_smallS < -700.0f) g_smallS =  700.0f; }
    else                    { if (g_smallS >  700.0f) g_smallS = -700.0f; }

    updateStars();                       // twinkle
}

// ----------------------------------------------------------------------------
//  display / reshape / keyboard
// ----------------------------------------------------------------------------
static void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawBackground(1.0f);
    drawMainMeteor();
    if (g_mainFade > 0.0f)
        drawBackground(g_mainFade);      // fades the comet out by painting the sky over it
    drawStars();
    drawClouds();                        // clouds sit in front of the comet tail
    drawSmallMeteor();

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h < 1) h = 1;
    // keep a 16:9 picture, add dark bars if the window has another shape
    const float target = 16.0f / 9.0f;
    int vw = w, vh = (int)(w / target);
    if (vh > h) { vh = h; vw = (int)(h * target); }
    glViewport((w - vw) / 2, (h - vh) / 2, vw, vh);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WORLD_W, 0.0, WORLD_H);   // 2D orthographic view: 1 unit = 1 "pixel" of the 1280x720 scene
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
        case 27:  std::exit(0); break;                     // ESC
        case ' ': g_paused = !g_paused; break;             // pause / resume
        case 'r':
        case 'R': resetAnimation(); break;
    }
    glutPostRedisplay();
}

static void timer(int)
{
    updateAnimation();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);         // about 60 frames per second
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Night Sky Meteors - OpenGL (SPACE pause, R reset, ESC quit)");

    initializeOpenGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}