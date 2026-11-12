#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

using namespace std;

static const float WORLD_W = 1280.0f;
static const float WORLD_H = 720.0f;
static const float PI      = 3.14159265f;

static const float MAIN_ANGLE_DEG = 36.0f;     // direction of the tail
static const float MAIN_REF_X = 435.0f;    // starting pose of the head
static const float MAIN_REF_Y = 225.0f;
static const float MAIN_SPEED = -150.0f;   // negative = head moves down-left, positive = up-right
static const float TAIL_LEN = 1900.0f;
static const float TAIL_CURVE = 3.0e-5f;   // how much the tail bends
static const float MAIN_MAX_HW = 140.0f;    // widest half-width of the tail
static const float FADE_TIME = 1.2f;

static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static float smoothstep(float a, float b, float x)
{
    float t = clampf((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
static float deg2rad(float d) { return d * PI / 180.0f; }

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

static bool g_useVBO = false;

static bool loadVBOFunctions(){
    return false;
}
static void vboGen(GLuint *id){
    (void)id;
}
static void vboBind(GLuint id){
    (void)id;
}
static void vboData(VboSize bytes, const void *data){
    (void)bytes;
    (void)data;
}

struct Vtx   { float x, y, r, g, b, a; }; // position + colour
struct Range { GLint first; GLsizei count; GLenum mode; }; // one glDrawArrays call

static Vtx makeVtx(float x, float y, float r, float g, float b, float a){
    Vtx v; v.x = x; 
    v.y = y; 
    v.r = r; 
    v.g = g; 
    v.b = b; 
    v.a = a; 
    return v;
}
static Range makeRange(GLint first, GLsizei count, GLenum mode){
    Range r; 
    r.first = first; 
    r.count = count; 
    r.mode = mode; 
    return r;
}

static vector<Vtx> g_meteorVerts;
static vector<Range> g_meteorRanges;
static GLuint g_meteorVBO = 0;
static GLuint g_glowTex   = 0;

// Animation state
static bool  g_paused = false;
static float g_time = 0.0f; // animation clock (stops while paused)
static float g_mainS = 0.0f; // how far the head moved along the tail axis
static bool  g_mainFading = false;
static float g_mainFade = 0.0f; // 0 = visible, 1 = hidden

static void initializeOpenGL();
static void setupTextures();
static void createMeteorGeometry();
static void drawMainMeteor();
static void updateAnimation();
static void display();
static void reshape(int w, int h);
static void keyboard(unsigned char key, int x, int y);


static void drawTexturedQuad(float s, float r, float g, float b, float a){
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-s,  s);
    glEnd();
}

static void mainHeadPos(float &x, float &y){
    float a = deg2rad(MAIN_ANGLE_DEG);
    x = MAIN_REF_X + cosf(a) * g_mainS;
    y = MAIN_REF_Y + sinf(a) * g_mainS;
}

static float mainPulse(){
    return 1.0f + 0.12f * sinf(g_time * 6.0f) + 0.04f * sinf(g_time * 15.0f);
}

static void initializeOpenGL(){
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black screen
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glShadeModel(GL_SMOOTH);

    g_useVBO = loadVBOFunctions();
    if (!g_useVBO)
        printf("VBO functions not available - using ordinary vertex arrays instead.\n");

    setupTextures();
    createMeteorGeometry();
}

static void setupTextures(){
    const int N = 64;
    vector<unsigned char> px(N * N * 4);
    for (int j = 0; j < N; ++j){
        for (int i = 0; i < N; ++i){
            float u = ((i + 0.5f) / N) * 2.0f - 1.0f;
            float v = ((j + 0.5f) / N) * 2.0f - 1.0f;
            float d2 = u * u + v * v;
            float a  = (d2 < 1.0f) ? expf(-5.0f * d2) * (1.0f - d2) : 0.0f;
            unsigned char *p = &px[(j * N + i) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = (unsigned char)(clampf(a * 1.6f, 0.0f, 1.0f) * 255.0f);
        }
    }
    glGenTextures(1, &g_glowTex);
    glBindTexture(GL_TEXTURE_2D, g_glowTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, &px[0]);
}

struct LayerDef { 
    float r, g, b, a, w, off; 
};

static void addMainLayer(const LayerDef &d){
    const int N = 90;
    vector<Vtx> top, mid, bot;
    for (int i = 0; i <= N; ++i){
        float t    = (float)i / (float)N;
        float x    = t * TAIL_LEN;
        float prof = 0.012f + 0.988f * powf(t, 1.2f); // thin at the head, wide far away
        float cy   = -TAIL_CURVE * x * x; // gentle bend
        float hw   = MAIN_MAX_HW * d.w * prof;
        float yc   = cy + d.off * MAIN_MAX_HW * prof;
        float a    = d.a * (0.25f + 0.75f * smoothstep(0.0f, 0.35f, t))
                         * (1.0f - smoothstep(0.88f, 1.0f, t));
        top.push_back(makeVtx(x, yc + hw, d.r, d.g, d.b, 0.0f)); // edge: transparent
        mid.push_back(makeVtx(x, yc,      d.r, d.g, d.b, a)); // centre: bright
        bot.push_back(makeVtx(x, yc - hw, d.r, d.g, d.b, 0.0f));
    }

    // upper half of the layer
    GLint first = (GLint)g_meteorVerts.size();
    for (size_t i = 0; i < top.size(); ++i){
        g_meteorVerts.push_back(top[i]);
        g_meteorVerts.push_back(mid[i]);
    }
    g_meteorRanges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));

    // lower half of the layer
    first = (GLint)g_meteorVerts.size();
    for (size_t i = 0; i < top.size(); ++i){
        g_meteorVerts.push_back(mid[i]);
        g_meteorVerts.push_back(bot[i]);
    }
    g_meteorRanges.push_back(makeRange(first, (GLsizei)(top.size() * 2), GL_TRIANGLE_STRIP));
}

static void createMeteorGeometry(){
    const LayerDef layers[] = {
        { 0.10f, 0.30f, 1.00f, 0.16f, 2.00f,  0.00f },  // wide transparent blue glow
        { 0.15f, 0.40f, 1.00f, 0.45f, 1.05f,  0.20f }, // blue body
        { 0.45f, 0.75f, 1.00f, 0.50f, 0.35f,  0.60f }, // light blue
        { 0.10f, 0.95f, 1.00f, 0.55f, 0.30f,  0.10f }, // cyan
        { 0.60f, 0.28f, 1.00f, 0.60f, 0.40f, -0.45f }, // purple
        { 1.00f, 0.25f, 0.75f, 0.65f, 0.22f, -0.82f }, // magenta edge
        { 0.85f, 0.95f, 1.00f, 0.95f, 0.09f,  0.85f } // white-blue core
    };
    for (size_t i = 0; i < sizeof(layers) / sizeof(layers[0]); ++i)
        addMainLayer(layers[i]);

    // Upload the tail to a Vertex Buffer Object
    if (g_useVBO){
        vboGen(&g_meteorVBO);
        vboBind(g_meteorVBO);
        vboData((VboSize)(g_meteorVerts.size() * sizeof(Vtx)), &g_meteorVerts[0]);
        vboBind(0);
    }
}

static void drawMainMeteor()
{
    float hx, hy;
    mainHeadPos(hx, hy);
    float p = mainPulse();

    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow

    glPushMatrix();
    glTranslatef(hx, hy, 0.0f); // move to the head
    glRotatef(MAIN_ANGLE_DEG, 0.0f, 0.0f, 1.0f); // point the tail the right way

    // --- tail layers from the VBO ---
    const char *base = 0;
    if (g_useVBO) vboBind(g_meteorVBO);
    else          base = (const char *)&g_meteorVerts[0];

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(Vtx), base);
    glColorPointer(4, GL_FLOAT, sizeof(Vtx), base + 2 * sizeof(float));
    for (size_t i = 0; i < g_meteorRanges.size(); ++i)
        glDrawArrays(g_meteorRanges[i].mode, g_meteorRanges[i].first, g_meteorRanges[i].count);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    if (g_useVBO) vboBind(0);

    // --- glowing head: several textured, additive quads ---
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_glowTex);
    drawTexturedQuad(110.0f * p, 0.15f, 0.35f, 1.00f, 0.35f); // big blue halo
    drawTexturedQuad(55.0f * p, 0.30f, 0.70f, 1.00f, 0.60f); // cyan glow
    drawTexturedQuad(24.0f * p, 0.70f, 0.95f, 1.00f, 0.90f); // bright inner glow
    drawTexturedQuad(9.0f * p, 1.00f, 1.00f, 1.00f, 1.00f); // white core
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}


static void resetAnimation(){
    g_time = 0.0f;
    g_mainS = 0.0f;
    g_mainFading = false;
    g_mainFade = 0.0f;
}

static void updateAnimation(){
    static int lastMs = -1;
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if (lastMs < 0) lastMs = nowMs;
    float dt = (nowMs - lastMs) / 1000.0f;
    lastMs = nowMs;
    if (dt > 0.1f) dt = 0.1f;
    if (g_paused) return;

    g_time  += dt;
    g_mainS += MAIN_SPEED * dt;

    // When the head leaves the screen, fade out, then restart on the far side
    bool out = (MAIN_SPEED < 0.0f) ? (g_mainS < -520.0f) : (g_mainS > 1100.0f);
    if (out && !g_mainFading){
        g_mainFading = true;
        g_mainFade   = 0.0f;
    }
    if (g_mainFading){
        g_mainFade += dt / FADE_TIME;
        if (g_mainFade >= 1.0f){
            g_mainFading = false;
            g_mainFade = 0.0f;
            g_mainS = (MAIN_SPEED < 0.0f) ? 1100.0f : -700.0f;
        }
    }
}

static void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawMainMeteor();

    glutSwapBuffers();
}

static void reshape(int w, int h){
    if (h < 1) h = 1;
    const float target = 16.0f / 9.0f; // keep a 16:9 picture
    int vw = w, vh = (int)(w / target);
    if (vh > h) { vh = h; vw = (int)(h * target); }
    glViewport((w - vw) / 2, (h - vh) / 2, vw, vh);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WORLD_W, 0.0, WORLD_H); // 2D view: 1 unit = 1 pixel of the 1280x720 scene
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void keyboard(unsigned char key, int, int){
    switch (key){
        case 27:  std::exit(0); break; // ESC
        case ' ': g_paused = !g_paused; break; // pause / resume
        case 'r':
        case 'R': resetAnimation(); break;
    }
    glutPostRedisplay();
}

static void timer(int){
    updateAnimation();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // about 60 frames per second
}

int main(int argc, char **argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Comet Only - Version 1 (SPACE pause, R reset, ESC quit)");

    initializeOpenGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}