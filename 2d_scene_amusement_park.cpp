#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <utility> /
using namespace std;


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ------------------- Globals -------------------
float wheelAngle = 0;
float car1X = -100, car2X = 900;
float sunX = 80, cloudX = -150;
float birdX = 900;
float wingAngle = 0;
bool wingUp = true;

// Night mode
bool isNight = false;

// Roller coaster
float cartT = 0.0f; // parametric position along track

// Fireworks
struct Firework {
    float x, y, radius;
    float r, g, b, alpha;
    bool active;
};
vector<Firework> fireworks;
vector<pair<int, int>> starPositions;

// Waving flags
float flagShear = 0.0f;
bool shearDir = true;
float flagTime = 0.0f;


// ------------------- Helper: Pixel -------------------
void setPixel(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

// ------------------- Bresenham Line -------------------
void drawLine(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        setPixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

// ------------------- Midpoint Circle -------------------
void drawCircle(int xc, int yc, int r) {
    int x = 0, y = r;
    int p = 1 - r;
    while (x <= y) {
        setPixel(xc + x, yc + y); setPixel(xc - x, yc + y);
        setPixel(xc + x, yc - y); setPixel(xc - x, yc - y);
        setPixel(xc + y, yc + x); setPixel(xc - y, yc + x);
        setPixel(xc + y, yc - x); setPixel(xc - y, yc - x);
        if (p < 0) p += 2 * x + 3;
        else { p += 2 * (x - y) + 5; y--; }
        x++;
    }
}

// ------------------- Scanline Fill -------------------
struct Edge { int ymax; float x, inv_m; };
vector<vector<Edge>> ET(800);
vector<Edge> AET;

void scanlineFill(vector<pair<int, int>> vertices, float r, float g, float b, float a) {
    int ymin = vertices[0].second, ymax = vertices[0].second;
    for (auto& v : vertices) { ymin = min(ymin, v.second); ymax = max(ymax, v.second); }
    ET.assign(800, {}); AET.clear();

    for (size_t i = 0; i < vertices.size(); i++) {
        int x0 = vertices[i].first, y0 = vertices[i].second;
        int x1 = vertices[(i + 1) % vertices.size()].first, y1 = vertices[(i + 1) % vertices.size()].second;
        if (y0 == y1) continue;
        int minY = min(y0, y1), maxY = max(y0, y1);
        float x_at_ymin = (y0 < y1) ? x0 : x1;
        float inv_m = (float)(x1 - x0) / (y1 - y0);
        ET[minY].push_back({ maxY,x_at_ymin,inv_m });
    }

    glColor4f(r, g, b, a);
    glBegin(GL_POINTS);
    for (int y = ymin; y <= ymax; y++) {
        for (auto e : ET[y]) AET.push_back(e);
        AET.erase(remove_if(AET.begin(), AET.end(), [y](Edge e) {return e.ymax == y; }), AET.end());
        sort(AET.begin(), AET.end(), [](Edge a, Edge b) {return a.x < b.x; });
        for (size_t i = 0; i + 1 < AET.size(); i += 2) {
            int xStart = (int)round(AET[i].x), xEnd = (int)round(AET[i + 1].x);
            for (int x = xStart; x <= xEnd; x++) glVertex2i(x, y);
        }
        for (auto& e : AET) e.x += e.inv_m;
    }
    glEnd();
}

// ------------------- Scene Elements -------------------
void drawGround() {
    vector<pair<int, int>> g = { {0,0},{900,0},{900,150},{0,150} };
    scanlineFill(g, 0.3f, 0.8f, 0.3f, 1.0f);
}

void drawRoad() {
    vector<pair<int, int>> r = { {0,60},{900,60},{900,120},{0,120} };
    scanlineFill(r, 0.2f, 0.2f, 0.2f, 1.0f);
    for (int i = 0; i < 9; i++) {
        vector<pair<int, int>> dash = { {50 + i * 100,85},{90 + i * 100,85},{90 + i * 100,95},{50 + i * 100,95} };
        scanlineFill(dash, 1, 1, 1, 1.0f);
    }
}

void drawTree(int x, int y) {
    vector<pair<int, int>> trunk = { {x,y},{x + 20,y},{x + 20,y + 60},{x,y + 60} };
    scanlineFill(trunk, 0.5f, 0.25f, 0.0f, 1.0f);
    vector<pair<int, int>> crown1 = { {x - 30,y + 60},{x + 50,y + 60},{x + 10,y + 120} };
    vector<pair<int, int>> crown2 = { {x - 25,y + 100},{x + 45,y + 100},{x + 10,y + 160} };
    scanlineFill(crown1, 0.0f, 0.7f, 0.0f, 1.0f);
    scanlineFill(crown2, 0.0f, 0.7f, 0.0f, 1.0f);
}

void drawSun() {
    int cx = (int)round(sunX), cy = 600, r = 40;
    drawCircle(cx, cy, r);
    vector<pair<int, int>> sunPoly;
    for (int a = 0; a < 360; a++) sunPoly.push_back({ cx + (int)(r * cos(a * M_PI / 180)), cy + (int)(r * sin(a * M_PI / 180)) });
    scanlineFill(sunPoly, 1, 1, 0, 1.0f);
    for (int i = 0; i < 360; i += 30) {
        int x1 = cx + 60 * cos(i * M_PI / 180), y1 = cy + 60 * sin(i * M_PI / 180);
        drawLine(cx, cy, x1, y1);
    }
}

void drawMoon() {
    int cx = 80, cy = 600, r = 30;
    vector<pair<int, int>> moonPoly;
    for (int a = 0; a < 360; a++) moonPoly.push_back({ cx + (int)(r * cos(a * M_PI / 180)), cy + (int)(r * sin(a * M_PI / 180)) });
    scanlineFill(moonPoly, 0.95f, 0.95f, 1.0f, 1.0f);
}

void drawStar(int x, int y) {
    vector<pair<int, int>> star = { {x,y},{x + 2,y + 6},{x + 6,y + 2},{x - 2,y + 2},{x + 4,y + 8} };
    scanlineFill(star, 1, 1, 1, 1.0f);
}

void drawCloud(int cx, int cy) {
    int rads[5] = { 20,30,25,20,25 };
    int off[5][2] = { {0,0},{30,10},{-30,10},{20,-10},{-20,-10} };
    for (int i = 0; i < 5; i++) {
        drawCircle(cx + off[i][0], cy + off[i][1], rads[i]);
        vector<pair<int, int>> poly;
        for (int a = 0; a < 360; a++) poly.push_back({ cx + off[i][0] + (int)(rads[i] * cos(a * M_PI / 180)), cy + off[i][1] + (int)(rads[i] * sin(a * M_PI / 180)) });
        scanlineFill(poly, 1, 1, 1, 1.0f);
    }
}

void drawTent() {
    vector<pair<int, int>> base = { {100,150},{300,150},{300,250},{100,250} };
    scanlineFill(base, 1, 0.5f, 0.5f, 1.0f);
    vector<pair<int, int>> roof = { {80,250},{320,250},{200,380} };
    scanlineFill(roof, 0.9f, 0.1f, 0.1f, 1.0f);
    vector<pair<int, int>> door = { {180,150},{220,150},{220,210},{180,210} };
    scanlineFill(door, 0.2f, 0.2f, 0.2f, 1.0f);

    // pole
    vector<pair<int, int>> pole = { {198,380},{202,380},{202,430},{198,430} };
    scanlineFill(pole, 0.3f, 0.3f, 0.3f, 1.0f);

    // waving circus flag
    float shear = 0.3f * sin(flagTime); // sinusoidal shear
    glPushMatrix();
    glTranslatef(202, 430, 0);
    GLfloat shearMat[16] = {
        1, shear, 0, 0,
        0, 1,      0, 0,
        0, 0,      1, 0,
        0, 0,      0, 1
    };
    glMultMatrixf(shearMat);
    glTranslatef(-202, -430, 0);

    vector<pair<int, int>> flag = { {202,430},{240,420},{202,410} };
    scanlineFill(flag, 1, 1, 0, 1.0f);
    glPopMatrix();
}

void drawHouse() {
    vector<pair<int, int>> base = { {400,150},{600,150},{600,250},{400,250} };
    scanlineFill(base, 0.6f, 0.4f, 0.2f, 1.0f);
    vector<pair<int, int>> roof = { {380,250},{620,250},{500,350} };
    scanlineFill(roof, 0.7f, 0, 0, 1.0f);
    vector<pair<int, int>> door = { {480,150},{520,150},{520,200},{480,200} };
    scanlineFill(door, 0.2f, 0.2f, 0.2f, 1.0f);

    // pole
    vector<pair<int, int>> pole = { {498,350},{502,350},{502,430},{498,430} };
    scanlineFill(pole, 0.3f, 0.3f, 0.3f, 1.0f);

    // waving house flag
    float shear = 0.3f * sin(flagTime + 1.5f); // phase shift so flags differ
    glPushMatrix();
    glTranslatef(502, 430, 0);
    GLfloat shearMat[16] = {
        1, shear, 0, 0,
        0, 1,      0, 0,
        0, 0,      1, 0,
        0, 0,      0, 1
    };
    glMultMatrixf(shearMat);
    glTranslatef(-502, -430, 0);

    vector<pair<int, int>> flag = { {502,430},{540,420},{502,410} };
    scanlineFill(flag, 1, 1, 0, 1.0f);
    glPopMatrix();
}


void drawWheel(int cx, int cy, int r) {
    vector<pair<int, int>> tyre;
    for (int a = 0; a < 360; a++) tyre.push_back({ cx + (int)(r * cos(a * M_PI / 180)), cy + (int)(r * sin(a * M_PI / 180)) });
    scanlineFill(tyre, 0, 0, 0, 1.0f);
    vector<pair<int, int>> hub;
    for (int a = 0; a < 360; a++) hub.push_back({ cx + (int)(r / 3 * cos(a * M_PI / 180)), cy + (int)(r / 3 * sin(a * M_PI / 180)) });
    scanlineFill(hub, 0.7f, 0.7f, 0.7f, 1.0f);

    glPushMatrix();
    glTranslatef((float)cx, (float)cy, 0.0f);
    glRotatef(wheelAngle, 0, 0, 1);
    glTranslatef(-(float)cx, -(float)cy, 0.0f);
    drawLine(cx - r, cy, cx + r, cy);
    drawLine(cx, cy - r, cx, cy + r);
    glPopMatrix();
}

void drawCar(float x, int color) {
    glPushMatrix();
    glTranslatef(x, 0, 0);
    if (color == 0) scanlineFill({ {100,120},{220,120},{220,180},{100,180} }, 1, 0, 0, 1.0f);
    else scanlineFill({ {100,120},{220,120},{220,180},{100,180} }, 0, 0, 1, 1.0f);
    scanlineFill({ {120,180},{200,180},{180,210},{140,210} }, 0.8f, 0.2f, 0.2f, 1.0f);
    scanlineFill({ {145,185},{175,185},{170,205},{150,205} }, 0.2f, 0.6f, 1.0f, 1.0f);
    drawWheel(140, 105, 15);
    drawWheel(180, 105, 15);
    glPopMatrix();
}

void drawFerrisWheel() {
    int cx = 700, cy = 300, r = 100;
    drawCircle(cx, cy, r);
    vector<pair<int, int>> stand1 = { {cx - 120,150},{cx - 60,150},{cx,cy - 30} };
    vector<pair<int, int>> stand2 = { {cx + 120,150},{cx + 60,150},{cx,cy - 30} };
    scanlineFill(stand1, 0.5f, 0.5f, 0.5f, 1.0f);
    scanlineFill(stand2, 0.5f, 0.5f, 0.5f, 1.0f);

    glPushMatrix();
    glTranslatef((float)cx, (float)cy, 0.0f);
    glRotatef(wheelAngle, 0, 0, 1);
    glTranslatef(-(float)cx, -(float)cy, 0.0f);
    for (int a = 0; a < 360; a += 45) {
        int x1 = cx + (int)(r * cos(a * M_PI / 180)), y1 = cy + (int)(r * sin(a * M_PI / 180));
        drawLine(cx, cy, x1, y1);
        vector<pair<int, int>> cab = { {x1 - 10,y1 - 10},{x1 + 10,y1 - 10},{x1 + 10,y1 + 10},{x1 - 10,y1 + 10} };
        scanlineFill(cab, 1, 0.5f, 0, 1.0f);
    }
    glPopMatrix();
}

void drawBird(int x, int y) {
    int wing = (int)(15 * sin(wingAngle * M_PI / 180));
    glColor3f(0, 0, 0);
    drawLine(x, y, x - 20, y + wing);
    drawLine(x, y, x + 20, y + wing);
}

// ------------------- Roller coaster -------------------
vector<pair<int, int>> coasterTrack = {
    {50,200},
    {150,300},
    {300,500},
    {450,350},
    {600,180},
    {800,250}
};

void drawCoasterTrack() {
    glColor3f(0, 0, 0);
    // Draw track lines
    for (size_t i = 0; i < coasterTrack.size() - 1; i++) {
        drawLine(coasterTrack[i].first, coasterTrack[i].second,
            coasterTrack[i + 1].first, coasterTrack[i + 1].second);
        // Add support pillars under each joint
        int baseY = 150; // ground level for supports
        drawLine(coasterTrack[i].first, coasterTrack[i].second,
            coasterTrack[i].first, baseY);
    }
    // Support under last point too
    int baseY = 150;
    drawLine(coasterTrack.back().first, coasterTrack.back().second,
        coasterTrack.back().first, baseY);
}

// linear interpolation along segments
pair<int, int> getCartPos(float t) {
    if (t < 0) t = 0;
    int seg = (int)floor(t);
    float localT = t - seg;
    if (seg >= (int)coasterTrack.size() - 1) seg = (int)coasterTrack.size() - 2;
    int x0 = coasterTrack[seg].first, y0 = coasterTrack[seg].second;
    int x1 = coasterTrack[seg + 1].first, y1 = coasterTrack[seg + 1].second;
    int x = (int)round((1 - localT) * x0 + localT * x1);
    int y = (int)round((1 - localT) * y0 + localT * y1);
    return { x,y };
}

// ------------------- Roller coaster -------------------
void drawCartTrain() {
    int numCarts = 5;      // number of connected carts
    float cartSpacing = 45; // distance between carts
    for (int i = 0; i < numCarts; i++) {
        float t = cartT - i * 0.2f; // stagger carts along the track
        if (t < 0) t = 0;
        int seg = (int)floor(t);
        if (seg >= (int)coasterTrack.size() - 1) seg = (int)coasterTrack.size() - 2;
        float localT = t - seg;
        int x0 = coasterTrack[seg].first, y0 = coasterTrack[seg].second;
        int x1 = coasterTrack[seg + 1].first, y1 = coasterTrack[seg + 1].second;
        // Position of the cart
        int cx = (int)round((1 - localT) * x0 + localT * x1);
        int cy = (int)round((1 - localT) * y0 + localT * y1);
        // Slope angle for tilt
        float angle = atan2(y1 - y0, x1 - x0) * 180.0f / M_PI;
        glPushMatrix();
        glTranslatef((float)cx, (float)cy, 0.0f);
        glRotatef(angle, 0, 0, 1); // tilt cart according to slope
        glTranslatef(-(float)cx, -(float)cy, 0.0f);
        // Base cart rectangle
        vector<pair<int, int>> cartBase = {
            {cx - 20, cy + 6},
            {cx + 20, cy + 6},
            {cx + 20, cy + 26},
            {cx - 20, cy + 26}
        };
        scanlineFill(cartBase, 0.8f, 0.0f, 0.0f, 1.0f);
        // Backrest
        vector<pair<int, int>> backrest = {
            {cx - 20, cy + 26},
            {cx + 20, cy + 26},
            {cx + 15, cy + 40},
            {cx - 15, cy + 40}
        };
        scanlineFill(backrest, 0.6f, 0.0f, 0.0f, 1.0f);
        // Safety bar
        vector<pair<int, int>> bar = {
            {cx - 18, cy + 18},
            {cx + 18, cy + 18},
            {cx + 18, cy + 22},
            {cx - 18, cy + 22}
        };
        scanlineFill(bar, 0.2f, 0.2f, 0.2f, 1.0f);
        // Wheels
        drawWheel(cx - 10, cy + 6, 6);
        drawWheel(cx + 10, cy + 6, 6);
        glPopMatrix();
    }
}

// ------------------- Fireworks -------------------
void spawnFirework() {
    Firework fw;
    fw.x = rand() % 700 + 80;
    fw.y = rand() % 250 + 380;
    fw.radius = 1.0f;
    fw.r = (rand() % 100) / 100.0f;
    fw.g = (rand() % 100) / 100.0f;
    fw.b = (rand() % 100) / 100.0f;
    fw.alpha = 1.0f;
    fw.active = true;

    fireworks.push_back(fw);
}

void drawFireworks() {
    for (auto& fw : fireworks) {
        if (!fw.active) continue;
        vector<pair<int, int>> poly;
        for (int a = 0; a < 360; a++) {
            poly.push_back({ fw.x + (int)round(fw.radius * cos(a * M_PI / 180.0)), fw.y + (int)round(fw.radius * sin(a * M_PI / 180.0)) });
        }
        scanlineFill(poly, fw.r, fw.g, fw.b, fw.alpha);
    }
}
// ------------------- Display -------------------
void display() {

    if (isNight) glClearColor(0.02f, 0.02f, 0.15f, 1.0f);
    else glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    drawGround();
    drawRoad();
    drawTent();
    drawHouse();
    drawTree(50, 150);
    drawTree(300, 150);
    drawFerrisWheel();
    if (isNight) {
        drawMoon();

        for (const auto& pos : starPositions) {
            drawStar(pos.first, pos.second);
        }
        drawFireworks();
    }
    else {
        drawSun();
        drawCloud(200 + (int)cloudX, 600);
        drawCloud(500 + (int)cloudX, 550);
    }
    drawCoasterTrack();
    drawCartTrain();
    drawCar(car1X, 0);
    drawCar(car2X, 1);
    drawBird((int)birdX, 600);
    drawBird((int)birdX + 60, 620);
    drawBird((int)birdX + 120, 610);
    glFlush();
}

// ------------------- Animation -------------------
void update(int) {

    wheelAngle += 3.0f; if (wheelAngle > 360.0f) wheelAngle -= 360.0f;
    sunX += 0.2f; if (sunX > 900) sunX = 0;
    cloudX += 1.0f; if (cloudX > 1100) cloudX = -200;
    car1X += 3.0f; if (car1X > 900) car1X = -200;
    car2X -= 3.0f; if (car2X < -200) car2X = 900;
    birdX -= 4.0f; if (birdX < -200) birdX = 900;
    if (wingUp) { wingAngle += 5.0f; if (wingAngle > 30.0f) wingUp = false; }
    else { wingAngle -= 5.0f; if (wingAngle < -30.0f) wingUp = true; }

    cartT += 0.02f;
    if (cartT > (float)(coasterTrack.size() - 1)) cartT = 0.0f;

    if (isNight) {
        if (rand() % 100 < 4) spawnFirework();
        for (auto& fw : fireworks) {
            if (!fw.active) continue;
            fw.radius += 1.2f;
            fw.alpha -= 0.02f;
            if (fw.radius > 45.0f || fw.alpha <= 0) {
                fw.active = false;
            }
        }

        fireworks.erase(remove_if(fireworks.begin(), fireworks.end(), [](const Firework& f) { return !f.active; }), fireworks.end());
    }
    else {

        for (auto& fw : fireworks) fw.active = false;
        fireworks.clear();
    }



    if (shearDir) { flagShear += 0.02f; if (flagShear > 0.3f) shearDir = false; }
    else { flagShear -= 0.02f; if (flagShear < -0.3f) shearDir = true; }

    flagTime += 0.1f;


    glutPostRedisplay();
    glutTimerFunc(30, update, 0);
}
// ------------------- Keyboard -------------------
void keyboard(unsigned char key, int x, int y) {
    if (key == 'n' || key == 'N') {
        isNight = true;

        for (int i = 0; i < 4; i++) spawnFirework();
    }
    else if (key == 'd' || key == 'D') {
        isNight = false;
    }
}
// ------------------- Init -------------------
void init() {
    srand((unsigned int)time(NULL));
    fireworks.clear();


    for (int i = 0; i < 50; ++i) {

        starPositions.push_back({ rand() % 900, rand() % 300 + 400 });
    }

    glPointSize(1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
    gluOrtho2D(0, 900, 0, 700);
}
// ------------------- Main -------------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(900, 700);
    glutCreateWindow("Amusement Park Scene");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(30, update, 0);
    glutMainLoop();
    return 0;
}

