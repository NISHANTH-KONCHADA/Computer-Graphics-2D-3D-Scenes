
// CinematicStation.cpp
// COMPILE: g++ CinematicStation_ManualTF.cpp -lGL -lGLU -lglut -o CinematicStation

#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#define DEG2RAD(x) ((x) * M_PI / 180.0f)

// ============================================================================
// FROM-SCRATCH 3D MATH AND TRANSFORMATION LIBRARY
// ============================================================================

struct Vec3 {
    float x, y, z;
    Vec3(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f) : x(x_), y(y_), z(z_) {}
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    float length() const { return sqrtf(x * x + y * y + z * z); }
    Vec3 normalize() const {
        float len = length();
        return len > 0.0001f ? Vec3(x / len, y / len, z / len) : Vec3(0.0f, 0.0f, 0.0f);
    }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
};

struct Matrix4 {
    float m[16]; // Column-major order for OpenGL

    Matrix4() { loadIdentity(); }

    void loadIdentity() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Matrix4 createTranslation(float x, float y, float z) {
        Matrix4 mat;
        mat.m[12] = x; mat.m[13] = y; mat.m[14] = z;
        return mat;
    }

    static Matrix4 createScale(float sx, float sy, float sz) {
        Matrix4 mat;
        mat.m[0] = sx; mat.m[5] = sy; mat.m[10] = sz;
        return mat;
    }

    static Matrix4 createRotation(float angle, float x, float y, float z) {
        Matrix4 mat;
        float rad = DEG2RAD(angle);
        float c = cosf(rad);
        float s = sinf(rad);
        Vec3 axis = Vec3(x, y, z).normalize();
        float one_minus_c = 1.0f - c;
        mat.m[0] = axis.x * axis.x * one_minus_c + c;
        mat.m[1] = axis.y * axis.x * one_minus_c + axis.z * s;
        mat.m[2] = axis.z * axis.x * one_minus_c - axis.y * s;
        mat.m[4] = axis.x * axis.y * one_minus_c - axis.z * s;
        mat.m[5] = axis.y * axis.y * one_minus_c + c;
        mat.m[6] = axis.z * axis.y * one_minus_c + axis.x * s;
        mat.m[8] = axis.x * axis.z * one_minus_c + axis.y * s;
        mat.m[9] = axis.y * axis.z * one_minus_c - axis.x * s;
        mat.m[10] = axis.z * axis.z * one_minus_c + c;
        return mat;
    }

    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[j + k * 4] * other.m[i * 4 + k];
                }
                result.m[j + i * 4] = sum;
            }
        }
        return result;
    }
};

Matrix4 modelViewMatrix;
std::vector<Matrix4> matrixStack;

void custom_push_matrix() { matrixStack.push_back(modelViewMatrix); }
void custom_pop_matrix() { modelViewMatrix = matrixStack.back(); matrixStack.pop_back(); }
void custom_load_identity() { modelViewMatrix.loadIdentity(); }
void custom_translate(float x, float y, float z) { modelViewMatrix = modelViewMatrix * Matrix4::createTranslation(x, y, z); }
void custom_rotate(float angle, float x, float y, float z) { modelViewMatrix = modelViewMatrix * Matrix4::createRotation(angle, x, y, z); }
void custom_scale(float sx, float sy, float sz) { modelViewMatrix = modelViewMatrix * Matrix4::createScale(sx, sy, sz); }

Matrix4 custom_look_at(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = (center - eye).normalize();
    Vec3 s = f.cross(up).normalize();
    Vec3 u = s.cross(f);
    Matrix4 viewMatrix;
    viewMatrix.m[0] = s.x; viewMatrix.m[4] = s.y; viewMatrix.m[8] = s.z;
    viewMatrix.m[1] = u.x; viewMatrix.m[5] = u.y; viewMatrix.m[9] = u.z;
    viewMatrix.m[2] = -f.x; viewMatrix.m[6] = -f.y; viewMatrix.m[10] = -f.z;
    return viewMatrix * Matrix4::createTranslation(-eye.x, -eye.y, -eye.z);
}

// ============================================================================
// MODIFIED TO USE CUSTOM TRANSFORMATIONS
// ============================================================================

// ---------- Scene parameters ----------
float cameraAngle = 20.0f;
float cameraRadius = 65.0f;
float baseCameraHeight = 18.0f;
float cameraHeight = 18.0f;

float trainPos = 120.0f;
float trainSpeed = 0.45f;
float signRotation = 0.0f; 

int windowWidth = 1280;
int windowHeight = 720;

// ---------- Structures for scene objects ----------
struct Smoke { float x, y, z, r, life, initialLife; };
std::vector<Smoke> smokeParticles;
struct Passenger { float x, z, phase; bool standing; };
std::vector<Passenger> passengers;
struct Tree { float x, z; };
std::vector<Tree> trees;

// ---------- Utility helpers ----------
void setMaterial(const float ambient[4], const float diffuse[4], const float specular[4], float shininess) {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

// Draw functions now use custom transforms
void drawBox(float sx, float sy, float sz) {
    custom_push_matrix();
    custom_scale(sx, sy, sz);
    glLoadMatrixf(modelViewMatrix.m);
    glutSolidCube(1.0f);
    custom_pop_matrix();
}

void drawCylinder(float radius, float height, int slices = 16) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluCylinder(q, radius, radius, height, slices, 1);
    gluDeleteQuadric(q);
}

void drawShadow(float radius) {
    custom_push_matrix();
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    custom_scale(1.0f, 0.01f, 1.0f);
    glLoadMatrixf(modelViewMatrix.m);
    glutSolidSphere(radius, 16, 8);
    custom_pop_matrix();
}

// ---------- Scene setup (Lighting, Fog) ----------
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat diffuse[] = { 1.0f, 0.95f, 0.8f, 1.0f };
    GLfloat specular[] = { 0.7f, 0.7f, 0.6f, 1.0f };
    GLfloat pos[] = { 60.0f, 60.0f, 20.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

void initFog() {
    glEnable(GL_FOG);
    GLfloat fogColor[] = { 0.75f, 0.85f, 0.95f, 1.0f };
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 0.006f);
}

// ---------- Drawing Functions ----------
void drawGround() {
    custom_push_matrix();
    glLoadMatrixf(modelViewMatrix.m);
    float amb[] = { 0.08f, 0.25f, 0.08f, 1.0f };
    float dif[] = { 0.12f, 0.45f, 0.12f, 1.0f };
    float spec[] = { 0.02f, 0.02f, 0.02f, 1.0f };
    setMaterial(amb, dif, spec, 5.0f);
    glColor3f(0.12f, 0.45f, 0.12f);
    glBegin(GL_QUADS);
    glVertex3f(-300.0f, 0.0f, -200.0f);
    glVertex3f(300.0f, 0.0f, -200.0f);
    glVertex3f(300.0f, 0.0f, 200.0f);
    glVertex3f(-300.0f, 0.0f, 200.0f);
    glEnd();
    custom_pop_matrix();

    for (int i = -1; i <= 1; ++i) {
        custom_push_matrix();
        custom_translate(i * 80.0f, 0.0f, -150.0f);
        custom_scale(90.0f, 1.0f, 40.0f);
        glLoadMatrixf(modelViewMatrix.m);
        glColor3f(0.14f, 0.35f, 0.14f);
        glutSolidSphere(1.5f, 24, 12);
        custom_pop_matrix();
    }
}

void drawPlatform() {
    custom_push_matrix();
    glLoadMatrixf(modelViewMatrix.m);
    float amb[] = { 0.18f, 0.18f, 0.18f, 1.0f };
    float dif[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    float spec[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    setMaterial(amb, dif, spec, 10.0f);
    glColor3f(0.65f, 0.65f, 0.65f);
    glBegin(GL_QUADS);
    glVertex3f(-200.0f, 0.01f, 6.0f);
    glVertex3f(200.0f, 0.01f, 6.0f);
    glVertex3f(200.0f, 0.01f, 20.0f);
    glVertex3f(-200.0f, 0.01f, 20.0f);
    glEnd();
    glColor3f(1.0f, 0.93f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(-200.0f, 0.02f, 5.6f);
    glVertex3f(200.0f, 0.02f, 5.6f);
    glVertex3f(200.0f, 0.02f, 6.0f);
    glVertex3f(-200.0f, 0.02f, 6.0f);
    glEnd();
    custom_pop_matrix();
}

void drawTracks() {
    custom_push_matrix();
    float amb[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    float dif[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    setMaterial(amb, dif, spec, 100.0f);
    glColor3f(0.3f, 0.3f, 0.3f);
    custom_push_matrix();
    custom_translate(0.0f, 0.2f, -1.0f);
    drawBox(600.0f, 0.2f, 0.2f);
    custom_pop_matrix();
    custom_push_matrix();
    custom_translate(0.0f, 0.2f, 1.0f);
    drawBox(600.0f, 0.2f, 0.2f);
    custom_pop_matrix();
    float amb_s[] = { 0.1f, 0.05f, 0.02f, 1.0f };
    float dif_s[] = { 0.36f, 0.22f, 0.12f, 1.0f };
    float spec_s[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    setMaterial(amb_s, dif_s, spec_s, 10.0f);
    glColor3f(0.36f, 0.22f, 0.12f);
    for (float x = -300.0f; x <= 300.0f; x += 4.0f) {
        custom_push_matrix();
        custom_translate(x, 0.1f, 0.0f);
        drawBox(1.0f, 0.15f, 5.0f);
        custom_pop_matrix();
    }
    custom_pop_matrix();
}

void drawTree(float x, float z) {
    glDisable(GL_LIGHTING);
    custom_push_matrix();
    custom_translate(x, 0.02f, z);
    drawShadow(1.2f);
    custom_pop_matrix();
    glEnable(GL_LIGHTING);

    custom_push_matrix();
    custom_translate(x, 0.0f, z);
    glColor3f(0.35f, 0.18f, 0.07f);
    drawBox(0.4f, 1.5f, 0.4f);
    custom_translate(0.0f, 1.9f, 0.0f);
    glLoadMatrixf(modelViewMatrix.m);
    glColor3f(0.06f, 0.45f, 0.08f);
    glutSolidSphere(1.1f, 18, 10);
    custom_translate(0.4f, -0.3f, 0.3f);
    glLoadMatrixf(modelViewMatrix.m);
    glutSolidSphere(0.8f, 18, 10);
    custom_pop_matrix();
}

void drawPassenger(const Passenger& p) {
    glDisable(GL_LIGHTING);
    custom_push_matrix();
    custom_translate(p.x, 0.03f, p.z);
    drawShadow(0.3f);
    custom_pop_matrix();
    glEnable(GL_LIGHTING);

    custom_push_matrix();
    custom_translate(p.x, 0.8f, p.z);
    float bob = (p.standing) ? 0.0f : sinf(p.phase) * 0.08f;
    custom_translate(0.0f, bob, 0.0f);
    custom_scale(0.8f, 0.8f, 0.8f);
    glLoadMatrixf(modelViewMatrix.m);
    glColor3f(0.1f, 0.1f, 0.1f);
    custom_push_matrix();
    custom_translate(0.0f, 0.3f, 0.0f);
    glLoadMatrixf(modelViewMatrix.m);
    glutSolidSphere(0.22f, 8, 6);
    custom_pop_matrix();
    drawBox(0.36f, 0.6f, 0.18f);
    custom_push_matrix();
    custom_translate(0.0f, -0.6f, 0.0f);
    drawBox(0.12f, 0.6f, 0.12f);
    custom_translate(0.16f, 0.0f, 0.0f);
    drawBox(0.12f, 0.6f, 0.12f);
    custom_pop_matrix();
    custom_pop_matrix();
}


void drawRotatingSign() {
    custom_push_matrix();
    custom_translate(30.0f, 0.0f, 15.0f); // Position on the platform

    // Tall stand/post
    custom_push_matrix();
    custom_translate(0.0f, 3.5f, 0.0f); // Center the post
    glColor3f(0.3f, 0.3f, 0.3f);
    drawBox(0.4f, 7.0f, 0.4f);
    custom_pop_matrix();

    // Rotating sign part
    custom_translate(0.0f, 7.5f, 0.0f); // Position sign on top of the post
    custom_rotate(signRotation, 0.0f, 1.0f, 0.0f); // Apply rotation
    glColor3f(0.8f, 0.8f, 0.6f);
    drawBox(3.0f, 1.5f, 0.2f); // The sign board

    custom_pop_matrix();
}


void spawnSmoke(float x, float y, float z) {
    Smoke s; s.x = x; s.y = y; s.z = z;
    s.r = 0.6f + static_cast<float>(rand() % 50) / 100.0f;
    s.life = 1.4f + static_cast<float>(rand() % 100) / 100.0f;
    s.initialLife = s.life;
    smokeParticles.push_back(s);
}

void updateSmoke() {
    for (size_t i = 0; i < smokeParticles.size(); ) {
        smokeParticles[i].y += 0.12f;
        smokeParticles[i].x += 0.04f + static_cast<float>(rand() % 20) / 200.0f;
        smokeParticles[i].z += (static_cast<float>(rand() % 20) / 100.0f) - 0.1f;
        smokeParticles[i].r += 0.01f;
        smokeParticles[i].life -= 0.02f;
        if (smokeParticles[i].life <= 0.0f) smokeParticles.erase(smokeParticles.begin() + i);
        else ++i;
    }
}

void drawSmoke() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    for (auto& s : smokeParticles) {
        float alpha = (s.life / s.initialLife) * 0.6f;
        glColor4f(0.6f, 0.6f, 0.6f, alpha);
        custom_push_matrix();
        custom_translate(s.x, s.y, s.z);
        glLoadMatrixf(modelViewMatrix.m);
        glutSolidSphere(s.r * 0.8f, 12, 8);
        custom_pop_matrix();
    }
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

void drawEngine(float baseX, float zOffset) {
    custom_push_matrix();
    custom_translate(baseX, 1.2f, zOffset);
    glColor3f(0.78f, 0.14f, 0.14f);
    drawBox(10.0f, 1.6f, 3.2f);
    glColor3f(0.6f, 0.05f, 0.05f);
    custom_translate(2.8f, 0.8f, 0.0f);
    drawBox(3.4f, 2.0f, 3.0f);
    custom_translate(-6.8f, -0.1f, 0.0f);
    glColor3f(0.72f, 0.2f, 0.18f);
    drawBox(4.5f, 1.2f, 3.0f);
    custom_translate(1.5f, 1.3f, 0.0f);
    glLoadMatrixf(modelViewMatrix.m);
    glColor3f(0.2f, 0.2f, 0.2f);
    drawCylinder(0.45f, 1.2f, 12);
    custom_pop_matrix();
}

void drawCoach(float baseX, float zOffset, const float colorC[3]) {
    custom_push_matrix();
    custom_translate(baseX, 1.2f, zOffset);
    glColor3f(colorC[0], colorC[1], colorC[2]);
    drawBox(14.0f, 2.0f, 3.0f);
    glColor3f(0.88f, 0.95f, 1.0f);
    for (float x = -14.0f / 2.0f + 1.5f; x < 14.0f / 2.0f - 1.0f; x += 3.0f) {
        custom_push_matrix();
        custom_translate(x, 0.2f, 1.55f);
        drawBox(1.8f, 0.9f, 0.06f);
        custom_translate(0.0f, 0.0f, -3.1f);
        drawBox(1.8f, 0.9f, 0.06f);
        custom_pop_matrix();
    }
    custom_pop_matrix();
}

void drawTrainAndReflection() {
    // Draw the actual train
    drawEngine(trainPos, 0.0f);
    const float c1[] = { 0.12f, 0.4f, 0.8f };
    const float c2[] = { 0.9f, 0.45f, 0.12f };
    const float c3[] = { 0.12f, 0.7f, 0.45f };
    drawCoach(trainPos + 16.0f, 0.0f, c1);
    drawCoach(trainPos + 34.0f, 0.0f, c2);
    drawCoach(trainPos + 52.0f, 0.0f, c3);
    drawCoach(trainPos + 70.0f, 0.0f, c1);

   
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    custom_push_matrix();
    custom_scale(1.0f, -1.0f, 1.0f); // Reflect across the ground plane
    custom_translate(0.0f, 0.2f, 0.0f); // Adjust position slightly to avoid z-fighting
    float amb_ref[] = { 0.1f, 0.1f, 0.1f, 0.4f }; // Semi-transparent material
    float dif_ref[] = { 0.2f, 0.2f, 0.2f, 0.4f };
    setMaterial(amb_ref, dif_ref, amb_ref, 10.0f);

    drawEngine(trainPos, 0.0f);
    drawCoach(trainPos + 16.0f, 0.0f, c1);
    drawCoach(trainPos + 34.0f, 0.0f, c2);
    drawCoach(trainPos + 52.0f, 0.0f, c3);
    drawCoach(trainPos + 70.0f, 0.0f, c1);

    custom_pop_matrix();
    glDisable(GL_BLEND);
}

void buildScene() {
    passengers.clear();
    for (int i = 0; i < 12; ++i) {
        Passenger p;
        p.x = -100.0f + static_cast<float>(rand() % 200);
        p.z = 10.0f + static_cast<float>(rand() % 80) * 0.12f;
        p.phase = static_cast<float>(rand() % 100) / 40.0f;
        p.standing = (rand() % 3 == 0);
        passengers.push_back(p);
    }
    trees.clear();
    for (int i = 0; i < 14; ++i) {
        Tree t;
        t.x = -220.0f + i * 36.0f + (rand() % 20 - 10);
        t.z = 40.0f + (rand() % 30);
        trees.push_back(t);
    }
}

void updateScene() {
    trainPos -= trainSpeed;
    if (trainPos < -300.0f) trainPos = 300.0f;
    signRotation += 1.0f; 
    if (signRotation > 360.0f) signRotation -= 360.0f;
    if ((rand() % 3) == 0) spawnSmoke(trainPos - 2.5f, 3.3f, 0.0f);
    updateSmoke();
    for (auto& p : passengers) {
        if (!p.standing) p.x += sinf(p.phase) * 0.05f + 0.02f;
        p.phase += 0.04f;
        if (p.x > 220.0f) p.x = -220.0f;
    }
}

// ---------- Main Render Loop ----------
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up camera using our custom lookAt function
    cameraHeight = baseCameraHeight + sinf(cameraAngle * 0.5f) * 1.5f;
    float camX = cameraRadius * cosf(cameraAngle * M_PI / 180.0f);
    float camZ = cameraRadius * sinf(cameraAngle * M_PI / 180.0f);
    modelViewMatrix = custom_look_at(Vec3(camX, cameraHeight, camZ), Vec3(0.0f, 2.5f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));

    drawGround();
    drawTracks();
    drawPlatform();
    drawRotatingSign(); // NEW

    for (auto& t : trees) drawTree(t.x, t.z);
    for (auto& p : passengers) drawPassenger(p);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); // Enable transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    custom_push_matrix();
    custom_translate(trainPos + 35.0f, 0.02f, 0.0f);
    custom_scale(80.0f, 1.0f, 3.5f); // Stretch the shadow to match the train's length
    drawShadow(1.0f);                // Draw a small shadow that will be stretched
    custom_pop_matrix();

    glDisable(GL_BLEND); // Disable transparency
    glEnable(GL_LIGHTING);

    drawTrainAndReflection(); // Draws both train and its reflection
    drawSmoke();

    glutSwapBuffers();
}

// ---------- GLUT Callbacks ----------
void reshape(int w, int h) {
    windowWidth = w; windowHeight = h;
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); // Use GLUT's perspective, as it's not a model transformation
    gluPerspective(45.0f, (float)w / (float)h, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27 || key == 'q') exit(0);
}

void timerFunc(int v) {
    cameraAngle += 0.04f;
    if (cameraAngle >= 360.0f) cameraAngle -= 360.0f;
    updateScene();
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0); // ~60 FPS
}

// ---------- Init and Main ----------
void initScene() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    initLighting();
    initFog();
    srand(static_cast<unsigned int>(time(nullptr)));
    buildScene();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Railway Station");
    initScene();
    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timerFunc, 0);
    glClearColor(0.75f, 0.85f, 0.95f, 1.0f);
    glutMainLoop();
    return 0;
}

