#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <cmath>
#include <iostream>
#include <math.h>
#include <cstdio>
#include <string.h>

#define PI 3.14159265358979323846


// ---------- DDA (Digital Differential Analyzer) line drawing ----------
// Works by sampling the longer of |dx|, |dy| as the number of steps and
// incrementing the other axis by a fractional amount each step.
void drawLineDDA(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    int steps = (fabs(dx) > fabs(dy)) ? (int)fabs(dx) : (int)fabs(dy);
    if (steps == 0) steps = 1;

    float xInc = dx / (float)steps;
    float yInc = dy / (float)steps;

    float x = x1;
    float y = y1;

    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++)
    {
        glVertex2f(x, y);
        x += xInc;
        y += yInc;
    }
    glEnd();
}

// ---------- Bresenham line drawing ----------
// Integer-only decision-parameter based line rasterization, generalised for
// all eight octants using sign vectors sx, sy.
void drawLineBresenham(float fx1, float fy1, float fx2, float fy2)
{
    int x1 = (int)(fx1 + 0.5f);
    int y1 = (int)(fy1 + 0.5f);
    int x2 = (int)(fx2 + 0.5f);
    int y2 = (int)(fy2 + 0.5f);

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    glBegin(GL_POINTS);
    while (true)
    {
        glVertex2i(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 <  dx)
        {
            err += dx;
            y1 += sy;
        }
    }
    glEnd();
}

// ---------- Mid-point Circle ----------
// 8-way symmetric plotting driven by the integer decision parameter p.
static void plotCirclePoints(int cx, int cy, int x, int y)
{
    glVertex2i(cx + x, cy + y);
    glVertex2i(cx - x, cy + y);
    glVertex2i(cx + x, cy - y);
    glVertex2i(cx - x, cy - y);
    glVertex2i(cx + y, cy + x);
    glVertex2i(cx - y, cy + x);
    glVertex2i(cx + y, cy - x);
    glVertex2i(cx - y, cy - x);
}

void drawCircleMidpoint(float cxf, float cyf, float rf)
{
    int cx = (int)(cxf + 0.5f);
    int cy = (int)(cyf + 0.5f);
    int r  = (int)(rf  + 0.5f);

    int x = 0;
    int y = r;
    int p = 1 - r;

    glBegin(GL_POINTS);
    plotCirclePoints(cx, cy, x, y);
    while (x < y)
    {
        x++;
        if (p < 0)
            p += 2 * x + 1;
        else
        {
            y--;
            p += 2 * (x - y) + 1;
        }
        plotCirclePoints(cx, cy, x, y);
    }
    glEnd();
}

// Filled circle built on top of the mid-point algorithm: for every scan
// pair produced by the algorithm we draw a horizontal span between the
// symmetric points, giving a filled disc.
void drawFilledCircleMidpoint(float cxf, float cyf, float rf,
                              int R, int G, int B)
{
    int cx = (int)(cxf + 0.5f);
    int cy = (int)(cyf + 0.5f);
    int r  = (int)(rf  + 0.5f);

    glColor3ub(R, G, B);

    int x = 0;
    int y = r;
    int p = 1 - r;
    while (x <= y)
    {
        glBegin(GL_LINES);
        glVertex2i(cx - x, cy + y);
        glVertex2i(cx + x, cy + y);
        glVertex2i(cx - x, cy - y);
        glVertex2i(cx + x, cy - y);
        glVertex2i(cx - y, cy + x);
        glVertex2i(cx + y, cy + x);
        glVertex2i(cx - y, cy - x);
        glVertex2i(cx + y, cy - x);
        glEnd();

        x++;
        if (p < 0)
            p += 2 * x + 1;
        else
        {
            y--;
            p += 2 * (x - y) + 1;
        }
    }
}

// ============================================================================
//  2D TRANSFORMATIONS (built as column-major 4x4 matrices and pushed into
//  the current OpenGL modelview stack via glMultMatrixf)
// ============================================================================

// Translation:  P' = P + T
void myTranslate(float tx, float ty)
{
    GLfloat m[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        tx,   ty, 0.0f, 1.0f
    };
    glMultMatrixf(m);
}

// Scaling about the origin
void myScale(float sx, float sy)
{
    GLfloat m[16] =
    {
        sx, 0.0f, 0.0f, 0.0f,
        0.0f,   sy, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glMultMatrixf(m);
}

// Rotation about the origin (angle in degrees, CCW)
void myRotate(float angleDeg)
{
    float a = angleDeg * (float)PI / 180.0f;
    float c = cosf(a);
    float s = sinf(a);
    GLfloat m[16] =
    {
        c,    s, 0.0f, 0.0f,
        -s,    c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glMultMatrixf(m);
}

// Shear: x' = x + shx*y,  y' = y + shy*x
void myShear(float shx, float shy)
{
    GLfloat m[16] =
    {
        1.0f,  shy, 0.0f, 0.0f,
        shx, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glMultMatrixf(m);
}

// Reflection:
//   'x' -> about the X axis   (y -> -y)
//   'y' -> about the Y axis   (x -> -x)
//   'o' -> about the origin   (x,y -> -x,-y)
void myReflect(char axis)
{
    float sx = 1.0f, sy = 1.0f;
    if (axis == 'x') sy = -1.0f;
    else if (axis == 'y') sx = -1.0f;
    else if (axis == 'o')
    {
        sx = -1.0f;
        sy = -1.0f;
    }
    myScale(sx, sy);
}


float cloudOffset = 0.0f;
float wheelRotationAngle = 0.0f;
float busPosition = 600.0f;
float carX = 600.0f;
float purpleCarPosition = 600.0f;
float purpleCarSpeed = 1.7f;
float boatX = -400.0f;
float boat2X = 600.0f;
float smokeY1 = 0.0f, smokeY2 = 2.0f, smokeY3 = 5.0f;
float smokeAngle1 = 0.0f, smokeAngle2 = 8.0f, smokeAngle3 = 9.0f;
float smokeSpeed = 0.5f;
float orangeBusPosition = 0.0f;
float orangeBusSpeed = 2.0f;

// ---------- River waves & boat pulsing (bigger <-> smaller) ----------
float wavePhase      = 0.0f;
float sailBoatPulse  = 1.0f;
float steamBoatPulse = 1.0f;

// ---------- Boat ping-pong motion (come near, turn, go far) ----------
// Each boat oscillates between a "near" endpoint (big on screen) and
// a "far" endpoint (small on screen).  dir == +1 means boat2X / boatX
// is currently increasing, -1 means decreasing.
int   sailBoatDir   = -1;
int   steamBoatDir  = +1;
const float sailNearX   = -350.0f, sailFarX   =  550.0f;
const float steamNearX  =  500.0f, steamFarX  = -350.0f;

void drawWaves()
{
    // Several sinusoidal water ripples spanning the river band. Each
    // wavy polyline is rasterised using the Bresenham algorithm.
    glColor3ub(255, 255, 255);

    float waveBaseY[] = { 15.0f, 40.0f, 65.0f, 90.0f, 115.0f };
    float waveAmp []  = {  3.5f,  4.5f,  3.0f,  4.0f,   3.5f };
    float waveFreq[]  = { 0.022f, 0.018f, 0.025f, 0.020f, 0.016f };

    for (int w = 0; w < 5; w++)
    {
        float by = waveBaseY[w];
        float a  = waveAmp[w];
        float k  = waveFreq[w];
        float phase = wavePhase + w * 0.9f;

        float prevX = -400.0f;
        float prevY = by + a * sinf(k * prevX + phase);
        for (float x = -396.0f; x <= 600.0f; x += 4.0f)
        {
            float y = by + a * sinf(k * x + phase);
            drawLineBresenham(prevX, prevY, x, y);
            prevX = x;
            prevY = y;
        }
    }
}

// ---------- Rain + Night mode (toggled by mouse clicks) ----------
bool  rainOn   = false;
bool  nightOn  = false;

const int STAR_COUNT = 80;
float starX[STAR_COUNT];
float starY[STAR_COUNT];
float starR[STAR_COUNT];

void initStars()
{
    for (int i = 0; i < STAR_COUNT; i++)
    {
        starX[i] = -400.0f + (rand() % 1000);
        // Keep stars above the night-overlay cap (y = 230) so nothing
        // dims them to a dull grey.
        starY[i] =  240.0f + (rand() % 110);
        starR[i] = 1.0f + (rand() % 2);
    }
}

// Stars are drawn before the clouds so clouds naturally occlude them.
void drawStars()
{
    if (!nightOn) return;
    for (int i = 0; i < STAR_COUNT; i++)
    {
        drawFilledCircleMidpoint(starX[i], starY[i], starR[i],
                                 255, 255, 220);
    }
}

void drawNightOverlay()
{
    if (!nightOn) return;

    // Dim the scene up to y = 230 so the tops of the 10-storey towers
    // and the large trees (which can reach y ~ 210) are no longer lit
    // by the sky. The upper sky band (where the moon and stars live)
    // remains untouched so those elements stay bright.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.02f, 0.15f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(-400.0f, -400.0f);
    glVertex2f( 600.0f, -400.0f);
    glVertex2f( 600.0f,  230.0f);
    glVertex2f(-400.0f,  230.0f);
    glEnd();
    glDisable(GL_BLEND);

    // Moon drawn after the darkening pass so it always renders bright.
    drawFilledCircleMidpoint(500.0f, 285.0f, 40.0f, 240, 240, 215);
    drawFilledCircleMidpoint(485.0f, 293.0f, 34.0f,  10,  14,  60);

    // Soft additive glow halos around each street-lamp bulb.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (int cx = -400; cx <= 600; cx += 200)
    {
        for (int d = -1; d <= 1; d += 2)
        {
            float lx = (float)cx + d * 50.0f;
            float ly = -15.0f;
            glColor4f(1.0f, 0.95f, 0.5f, 0.22f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(lx, ly);
            for (int a = 0; a <= 360; a += 12)
            {
                float ang = a * (float)PI / 180.0f;
                glVertex2f(lx + cosf(ang) * 28.0f,
                           ly + sinf(ang) * 28.0f);
            }
            glEnd();
        }
    }
    glDisable(GL_BLEND);
}

const int RAIN_COUNT = 220;
float rainX[RAIN_COUNT];
float rainY[RAIN_COUNT];
float rainLen[RAIN_COUNT];
float rainSpeed[RAIN_COUNT];

void initRain()
{
    for (int i = 0; i < RAIN_COUNT; i++)
    {
        rainX[i]     = -400.0f + (rand() % 1000);
        rainY[i]     = -400.0f + (rand() % 750);
        rainLen[i]   = 8.0f + (rand() % 8);
        rainSpeed[i] = 6.0f + (rand() % 6);
    }
}

void drawRain()
{
    if (!rainOn) return;
    glColor3ub(180, 210, 255);
    for (int i = 0; i < RAIN_COUNT; i++)
    {
        // Each drop is a short slanted line, drawn with Bresenham.
        drawLineBresenham(rainX[i], rainY[i],
                          rainX[i] - rainLen[i] * 0.3f,
                          rainY[i] - rainLen[i]);
    }
}

void drawFilledCircle(float cx, float cy, float r, int R, int G, int B)
{
    // Filled using the mid-point circle algorithm (8-way symmetric scan fill).
    drawFilledCircleMidpoint(cx, cy, r, R, G, B);
}

void drawRotatingWheel(float x_center, float y_center, float radius, int direction)
{
    glPushMatrix();
    glTranslatef(x_center, y_center, 0.0f);

    glColor3ub(20, 20, 20);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= 360; i++)
    {
        float angle = (i * 3.14159f) / 180.0f;
        glVertex2f(radius * cos(angle), radius * sin(angle));
    }
    glEnd();

    float rimRadius = radius * 0.65f;
    glColor3ub(180, 180, 180);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= 360; i++)
    {
        float angle = (i * 3.14159f) / 180.0f;
        glVertex2f(rimRadius * cos(angle), rimRadius * sin(angle));
    }
    glEnd();

    glPushMatrix();
    // Custom rotation matrix instead of glRotatef.
    myRotate(wheelRotationAngle * direction);
    glColor3ub(230, 230, 230);
    glLineWidth(2.0f);
    const int spokeCount = 8;
    for (int i = 0; i < spokeCount; i++)
    {
        float angle = (2.0f * PI * i) / spokeCount;
        // Draw each spoke using Bresenham's line algorithm.
        drawLineBresenham(0.0f, 0.0f,
                          (rimRadius - 1.5f) * cos(angle),
                          (rimRadius - 1.5f) * sin(angle));
    }
    glPopMatrix();

    glColor3ub(100, 100, 100);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= 360; i++)
    {
        float angle = (i * 3.14159f) / 180.0f;
        glVertex2f((radius * 0.14f) * cos(angle), (radius * 0.14f) * sin(angle));
    }
    glEnd();

    glPopMatrix();
}

void drawVerticalLine(float x, float y1, float y2)
{
    // Rasterised with DDA.
    drawLineDDA(x, y1, x, y2);
}

void drawRoadDash(float x1, float x2, float y)
{
    // Rasterised with Bresenham.
    drawLineBresenham(x1, y, x2, y);
}

void drawSky(float yOffset)
{
    glPushMatrix();
    glTranslatef(0.0f, yOffset, 0.0f);

    if (nightOn)
    {
        // Dark river / lower band
        glBegin(GL_POLYGON);
        glColor3ub(5, 10, 40);
        glVertex2f(-400.0f, -150.0f);
        glVertex2f(-400.0f,  150.0f);
        glVertex2f( 600.0f,  150.0f);
        glVertex2f( 600.0f, -150.0f);
        glEnd();

        // Night sky gradient (deeper at the top)
        glBegin(GL_POLYGON);
        glColor3ub(8, 10, 55);
        glVertex2f(-400.0f, 350.0f);
        glVertex2f( 600.0f, 350.0f);
        glColor3ub(20, 25, 80);
        glVertex2f( 600.0f, 150.0f);
        glVertex2f(-400.0f, 150.0f);
        glEnd();
        // (the moon itself is drawn later in drawNightOverlay, after the
        //  ground-darkening pass, so it always stays bright.)
    }
    else
    {
        glBegin(GL_POLYGON);
        glColor3ub(0, 191, 255);
        glVertex2f(-400.0f, -150.0f);
        glVertex2f(-400.0f, 150.0f);
        glVertex2f(600.0f, 150.0f);
        glVertex2f(600.0f, -150.0f);
        glEnd();

        glBegin(GL_POLYGON);
        glColor3ub(255, 255, 102);
        glVertex2f(-400.0f, 350.0f);
        glVertex2f(600.0f, 350.0f);
        glColor3ub(255, 153, 102);
        glVertex2f(600.0f, 150.0f);
        glVertex2f(-400.0f, 150.0f);
        glEnd();

        glBegin(GL_TRIANGLE_FAN);
        glColor3ub(255, 0, 0);
        glVertex2f(500.0f, 250.0f);
        for (int i = 0; i <= 360; i++)
        {
            float angle = i * 3.14159 / 180;
            float x = 500.0f + cos(angle) * 40.0f;
            float y = 250.0f + sin(angle) * 40.0f;
            glVertex2f(x, y);
        }
        glEnd();
    }
    glPopMatrix();
}

void drawCircle(float cx, float cy, float r, int rCol, int gCol, int bCol)
{
    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(rCol, gCol, bCol);
    glVertex2f(cx, cy);

    for (int i = 0; i <= 360; i++)
    {
        float a = i * 3.14159f / 180.0f;
        glVertex2f(cx + cos(a) * r, cy + sin(a) * r);
    }
    glEnd();
}

void drawSingleCloud(float x, float y, float scale)
{
    glPushMatrix();
    // Custom translation + scaling matrices (instead of glTranslatef/glScalef).
    myTranslate(x, y);
    myScale(scale, scale);

    drawCircle(0, 0, 25, 200, 205, 210);
    drawCircle(25, 5, 30, 200, 205, 210);
    drawCircle(50, 0, 22, 200, 205, 210);

    drawCircle(-5, 10, 22, 225, 230, 235);
    drawCircle(20, 18, 28, 230, 235, 240);
    drawCircle(45, 10, 20, 225, 230, 235);

    drawCircle(10, 25, 18, 255, 255, 255);
    drawCircle(30, 20, 16, 250, 250, 250);

    glPopMatrix();
}

void drawClouds(float yOffset)
{

    struct Cloud
    {
        float x, y;
        float scale;
        float speed;
    };

    Cloud clouds[12] =
    {
        {-300, 300, 1.0f, 0.4f},
        {-100, 320, 0.8f, 0.6f},
        {50, 280, 1.2f, 0.5f},
        {200, 310, 0.9f, 0.3f},
        {350, 290, 1.1f, 0.7f},
        {500, 330, 0.7f, 0.4f},
        {-400, 270, 1.3f, 0.6f},
        {-250, 340, 0.6f, 0.3f},
        {150, 260, 1.0f, 0.5f},
        {300, 350, 0.8f, 0.4f},
        {450, 275, 1.2f, 0.6f},
        {-150, 310, 0.9f, 0.5f}
    };

    for (int i = 0; i < 12; i++)
    {
        float x = clouds[i].x + cloudOffset * clouds[i].speed;

        drawSingleCloud(x, clouds[i].y, clouds[i].scale);

    }
}

void drawTallSkyTree(float x, float y)
{
    glPushMatrix();
    glTranslatef(0.0f, y, 0.0f);
    glLineWidth(5.0);
    glBegin(GL_LINES);
    glColor3ub(0, 0, 0);
    glVertex2f(x, 150);
    glVertex2f(x, 190);
    glEnd();

    glLineWidth(4.0);
    glBegin(GL_LINES);
    glVertex2f(x, 190);
    glVertex2f(x + 10, 205);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(x, 190);
    glVertex2f(x - 10, 200);
    glEnd();

    drawFilledCircle(x - 15, 205, 9, 0, 0, 0);
    drawFilledCircle(x + 10, 215, 10, 0, 0, 0);
    drawFilledCircle(x - 2, 225, 15, 0, 0, 0);
    glPopMatrix();
}

void drawSailBoat(float x, float y)
{
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    // Scale depends on the boat's current position: larger near the
    // viewer, smaller when it sails off toward the far shore.
    myScale(1.2f * sailBoatPulse, 1.2f * sailBoatPulse);
    // Horizontal reflection so the boat actually faces the direction
    // of travel after it turns around.
    if (sailBoatDir == +1)
        myReflect('y');
    glColor3ub(0, 0, 0);
    glBegin(GL_POLYGON);
    glVertex2f(-40, 0);
    glVertex2f(40, 0);
    glVertex2f(30, -15);
    glVertex2f(-30, -15);
    glEnd();

    glColor3ub(139, 69, 19);
    glBegin(GL_POLYGON);
    glVertex2f(-25, 0);
    glVertex2f(25, 0);
    glVertex2f(20, 5);
    glVertex2f(-20, 5);
    glEnd();

    glColor3ub(255, 255, 255);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, 5);
    glVertex2f(0, 30);
    glVertex2f(15, 30);
    glEnd();

    glColor3ub(139, 69, 19);
    glBegin(GL_QUADS);
    glVertex2f(3, 5);
    glVertex2f(3, 30);
    glVertex2f(8, 30);
    glVertex2f(8, 5);
    glEnd();

    glPopMatrix();
}

void drawSteamBoat(float x, float y)
{
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    // Scale depends on the boat's current position.
    myScale(1.5f * steamBoatPulse, 1.5f * steamBoatPulse);
    // Reflect horizontally when moving back toward the far shore.
    if (steamBoatDir == -1)
        myReflect('y');
    glColor3ub(101, 67, 33);
    glBegin(GL_POLYGON);
    glVertex2f(-50, 0);
    glVertex2f(50, 0);
    glVertex2f(40, -15);
    glVertex2f(-40, -15);
    glEnd();

    glColor3ub(169, 169, 169);
    glBegin(GL_QUADS);
    glVertex2f(-30, 10);
    glVertex2f(-30, 0);
    glVertex2f(30, 0);
    glVertex2f(30, 10);
    glEnd();

    glColor3ub(0, 0, 0);
    glBegin(GL_QUADS);
    glVertex2f(-20, 5);
    glVertex2f(-20, 3);
    glVertex2f(-15, 3);
    glVertex2f(-15, 5);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(-5, 5);
    glVertex2f(-5, 3);
    glVertex2f(0, 3);
    glVertex2f(0, 5);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(10, 5);
    glVertex2f(10, 3);
    glVertex2f(15, 3);
    glVertex2f(15, 5);
    glEnd();

    glColor3ub(255, 0, 0);
    glBegin(GL_QUADS);
    glVertex2f(-10, 20);
    glVertex2f(-10, 10);
    glVertex2f(0, 10);
    glVertex2f(0, 20);
    glEnd();

    // Smoke plume #1 : translation + rotation (myTranslate + myRotate).
    glPushMatrix();
    myTranslate(-5, 25 + smokeY1);
    myRotate(smokeAngle1);
    glColor3ub(169, 169, 169);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, 5);
    glVertex2f(-3, 0);
    glVertex2f(3, 0);
    glEnd();
    glPopMatrix();

    // Smoke plume #2 : shearing applied instead of rotation, so the plume
    // leans as it rises, as if blown sideways by the wind.
    glPushMatrix();
    myTranslate(5, 25 + smokeY2);
    myShear(smokeAngle2 * 0.02f, 0.0f);
    glColor3ub(169, 169, 169);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, 5);
    glVertex2f(-3, 0);
    glVertex2f(3, 0);
    glEnd();
    glPopMatrix();

    // Smoke plume #3 : rotation + reflection about the Y-axis, giving
    // the third puff a mirrored lean.
    glPushMatrix();
    myTranslate(10, 25 + smokeY3);
    myRotate(smokeAngle3);
    myReflect('y');
    glColor3ub(169, 169, 169);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, 5);
    glVertex2f(-3, 0);
    glVertex2f(3, 0);
    glEnd();
    glPopMatrix();

    glPopMatrix();
}

void drawGrass(float yOffset)
{
    glPushMatrix();
    glTranslatef(0.0f, yOffset, 0.0f);

    glColor3ub(0, 255, 0);
    glBegin(GL_POLYGON);
    glVertex2f(-400, 0);
    glVertex2f(600, 0);
    glVertex2f(600, -50);
    glVertex2f(-400, -50);
    glEnd();
    glPopMatrix();
}

void drawPineTree(float x, float y)
{
    glPushMatrix();
    glTranslatef(0.0f, y, 0.0f);

    glBegin(GL_POLYGON);
    glColor3ub(139, 69, 19);
    glVertex2f(x, 0);
    glVertex2f(x, 30);
    glVertex2f(x + 5, 30);
    glVertex2f(x + 5, 0);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(34, 139, 34);
    glVertex2f(x - 15, 30);
    glVertex2f(x + 20, 30);
    glVertex2f(x + 2.5, 60);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(x - 10, 45);
    glVertex2f(x + 15, 45);
    glVertex2f(x + 2.5, 75);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(x - 5, 60);
    glVertex2f(x + 10, 60);
    glVertex2f(x + 2.5, 90);
    glEnd();
    glPopMatrix();
}

void drawLargeTree(float x, float y)
{
    glLineWidth(12.5);
    glBegin(GL_LINES);
    glColor3ub(102, 67, 33);
    glVertex2f(x, y + 0.0f);
    glVertex2f(x, y + 120.0f);
    glEnd();

    glLineWidth(9.5);
    glBegin(GL_LINES);
    glVertex2f(x, y + 120.0f);
    glVertex2f(x + 20.0f, y + 160.0f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(x, y + 120.0f);
    glVertex2f(x - 10.0f, y + 140.0f);
    glEnd();

    drawFilledCircle(x - 20, y + 150, 24, 0, 100, 0);
    drawFilledCircle(x + 20, y + 170, 26, 0, 100, 0);
    drawFilledCircle(x, y + 190, 36, 0, 100, 0);
    drawFilledCircle(x - 30, y + 180, 24, 0, 100, 0);
}

void drawMediumTree(float x, float y)
{
    glPushMatrix();
    glTranslatef(0.0f, y, 0.0f);
    glLineWidth(10.0);
    glBegin(GL_LINES);
    glColor3ub(102, 67, 33);
    glVertex2f(x, 0.0f);
    glVertex2f(x, 100.0f);
    glEnd();

    glLineWidth(7.5);
    glBegin(GL_LINES);
    glVertex2f(x, 100.0f);
    glVertex2f(x + 15.0f, 130.0f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(x, 100.0f);
    glVertex2f(x - 10.0f, 120.0f);
    glEnd();

    drawFilledCircle(x - 20, 130, 18, 0, 100, 0);
    drawFilledCircle(x + 10, 145, 20, 0, 100, 0);
    drawFilledCircle(x, 165, 28, 0, 100, 0);
    glPopMatrix();
}

void drawMainRoad(float yOffset)
{
    glPushMatrix();
    glTranslatef(0.0f, yOffset, 0.0f);

    glBegin(GL_POLYGON);
    glColor3ub(32, 32, 32);
    glVertex2f(-400, -50);
    glVertex2f(600, -50);
    glVertex2f(600, -150);
    glVertex2f(-400, -150);
    glEnd();

    glColor3ub(160, 160, 160);
    glBegin(GL_POLYGON);
    glVertex2f(-400, -40);
    glVertex2f(600, -40);
    glVertex2f(600, -50);
    glVertex2f(-400, -50);
    glEnd();

    glColor3ub(255, 255, 255);
    for (float x = -390; x <= 590; x += 70)
    {
        drawVerticalLine(x, -50, -40);
    }

    glLineWidth(4.5);
    for (float x = -400; x <= 600; x += 80)
    {
        drawRoadDash(x, x + 40, -95);
    }

    glPopMatrix();
}

void drawLowerRoad(float yOffset)
{
    glPushMatrix();
    glTranslatef(0.0f, yOffset, 0.0f);

    glColor3ub(160, 160, 160);
    glBegin(GL_POLYGON);
    glVertex2f(-400, -200);
    glVertex2f(600, -200);
    glVertex2f(600, -215);
    glVertex2f(-400, -215);
    glEnd();

    glLineWidth(10.5);
    glColor3ub(255, 255, 255);
    for (float x = -380; x <= 600; x += 70)
    {
        drawVerticalLine(x, -200, -215);
    }

    glBegin(GL_POLYGON);
    glColor3ub(96, 96, 96);
    glVertex2f(-400.0f, -215.0f);
    glVertex2f(-400.0f, -400.0f);
    glVertex2f(600.0f, -400.0f);
    glVertex2f(600.0f, -215.0f);
    glEnd();

    glLineWidth(8.5);
    glColor3ub(255, 255, 255);
    for (float x = -380; x <= 600; x += 90)
    {
        drawRoadDash(x, x + 50, -300);
    }

    glPopMatrix();
}

void drawTenStoreyTower(float x, float y, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    glColor3ub(150, 150, 170);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(120.0f, 0.0f);
    glVertex2f(120.0f, 220.0f);
    glVertex2f(0.0f, 220.0f);
    glEnd();

    glLineWidth(1.5f);
    glColor3ub(90, 90, 110);
    for (int i = 1; i < 10; i++)
    {
        float fy = i * 22.0f;
        glBegin(GL_LINES);
        glVertex2f(0.0f, fy);
        glVertex2f(120.0f, fy);
        glEnd();
    }

    glColor3ub(190, 230, 255);
    for (int row = 0; row < 10; row++)
    {
        float wy = 4.0f + row * 22.0f;
        for (int col = 0; col < 4; col++)
        {
            float wx = 10.0f + col * 26.0f;
            glBegin(GL_QUADS);
            glVertex2f(wx, wy);
            glVertex2f(wx + 16.0f, wy);
            glVertex2f(wx + 16.0f, wy + 14.0f);
            glVertex2f(wx, wy + 14.0f);
            glEnd();
        }
    }

    glColor3ub(70, 50, 40);
    glBegin(GL_QUADS);
    glVertex2f(48.0f, 0.0f);
    glVertex2f(72.0f, 0.0f);
    glVertex2f(72.0f, 25.0f);
    glVertex2f(48.0f, 25.0f);
    glEnd();

    glPopMatrix();
}

void drawSixStoreyBuilding(float x, float y, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    glColor3ub(168, 166, 182);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(82.0f, 0.0f);
    glVertex2f(82.0f, 132.0f);
    glVertex2f(0.0f, 132.0f);
    glEnd();

    glLineWidth(1.2f);
    glColor3ub(98, 98, 118);
    for (int i = 1; i < 6; i++)
    {
        float fy = i * 22.0f;
        glBegin(GL_LINES);
        glVertex2f(0.0f, fy);
        glVertex2f(82.0f, fy);
        glEnd();
    }

    glColor3ub(188, 226, 248);
    for (int row = 0; row < 6; row++)
    {
        float wy = 4.0f + row * 22.0f;
        for (int col = 0; col < 3; col++)
        {
            float wx = 10.0f + col * 24.0f;
            glBegin(GL_QUADS);
            glVertex2f(wx, wy);
            glVertex2f(wx + 14.0f, wy);
            glVertex2f(wx + 14.0f, wy + 14.0f);
            glVertex2f(wx, wy + 14.0f);
            glEnd();
        }
    }

    glColor3ub(76, 54, 42);
    glBegin(GL_QUADS);
    glVertex2f(32.0f, 0.0f);
    glVertex2f(50.0f, 0.0f);
    glVertex2f(50.0f, 20.0f);
    glVertex2f(32.0f, 20.0f);
    glEnd();

    glPopMatrix();
}

void drawBus(float yOffset)
{
    glPushMatrix();
    glTranslatef(busPosition, -110.0f + yOffset, 0.0f);

    glColor3ub(111, 78, 55);
    glBegin(GL_POLYGON);
    glVertex2f(40.0f, 30.0f);
    glVertex2f(160.0f, 30.0f);
    glVertex2f(160.0f, 90.0f);
    glVertex2f(40.0f, 90.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(50.0f, 90.0f);
    glVertex2f(150.0f, 90.0f);
    glVertex2f(140.0f, 105.0f);
    glVertex2f(60.0f, 105.0f);
    glEnd();

    glColor3ub(200, 255, 255);

    glBegin(GL_POLYGON);
    glVertex2f(55.0f, 65.0f);
    glVertex2f(70.0f, 65.0f);
    glVertex2f(70.0f, 85.0f);
    glVertex2f(55.0f, 85.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(75.0f, 65.0f);
    glVertex2f(90.0f, 65.0f);
    glVertex2f(90.0f, 85.0f);
    glVertex2f(75.0f, 85.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(95.0f, 65.0f);
    glVertex2f(110.0f, 65.0f);
    glVertex2f(110.0f, 85.0f);
    glVertex2f(95.0f, 85.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(115.0f, 65.0f);
    glVertex2f(130.0f, 65.0f);
    glVertex2f(130.0f, 85.0f);
    glVertex2f(115.0f, 85.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(135.0f, 65.0f);
    glVertex2f(150.0f, 65.0f);
    glVertex2f(150.0f, 85.0f);
    glVertex2f(135.0f, 85.0f);
    glEnd();

    drawRotatingWheel(65.0f, 30.0f, 15.0f, -1);
    drawRotatingWheel(135.0f, 30.0f, 15.0f, -1);

    glPopMatrix();
}

void drawPurpleCar(float yOffset)
{
    glPushMatrix();
    glTranslatef(purpleCarPosition, -120.0f + yOffset, 0.0f);

    glBegin(GL_POLYGON);
    glColor3ub(102, 0, 102);
    glVertex2f(0, 0);
    glVertex2f(100, 0);
    glVertex2f(110, 30);
    glVertex2f(-10, 30);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(102, 0, 102);
    glVertex2f(20, 30);
    glVertex2f(90, 30);
    glVertex2f(75, 50);
    glVertex2f(35, 50);
    glEnd();

    glColor3ub(180, 220, 255);
    glBegin(GL_POLYGON);
    glVertex2f(25, 32);
    glVertex2f(53, 32);
    glVertex2f(53, 47);
    glVertex2f(35, 47);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(57, 32);
    glVertex2f(85, 32);
    glVertex2f(75, 47);
    glVertex2f(57, 47);
    glEnd();

    drawRotatingWheel(20.0f, 0.0f, 15.0f, -1);
    drawRotatingWheel(80.0f, 0.0f, 15.0f, -1);

    glPopMatrix();
}

void drawMedianTree(float x, float yOffset)
{
    glPushMatrix();
    glTranslatef(0.0f, yOffset, 0.0f);

    glColor3f(0.6f, 0.3f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x, -190);
    glVertex2f(x + 5, -190);
    glVertex2f(x + 5, -165);
    glVertex2f(x, -165);
    glEnd();

    glColor3f(0.0f, 0.5f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - 10, -165);
    glVertex2f(x + 15, -165);
    glVertex2f(x + 2, -140);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(x - 8, -158);
    glVertex2f(x + 13, -158);
    glVertex2f(x + 2, -130);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(x - 5, -150);
    glVertex2f(x + 10, -150);
    glVertex2f(x + 2, -120);
    glEnd();
    glPopMatrix();
}

void drawStreetLamp(float cx, float yOffset)
{
    glPushMatrix();
    glTranslatef(0.0f, yOffset, 0.0f);

    glBegin(GL_QUADS);
    glColor3ub(255, 215, 0);
    glVertex2f(cx - 5.0f, -195.0f);
    glVertex2f(cx + 5.0f, -195.0f);
    glVertex2f(cx + 5.0f, -100.0f);
    glVertex2f(cx - 5.0f, -100.0f);
    glEnd();

    glBegin(GL_LINES);
    glColor3ub(184, 134, 11);
    glVertex2f(cx, -100.0f);
    glVertex2f(cx, -50.0f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(cx, -50.0f);
    glVertex2f(cx + 50.0f, -25.0f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(cx, -50.0f);
    glVertex2f(cx - 50.0f, -25.0f);
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(255, 255, 255);
    glVertex2f(cx + 50.0f, -25.0f);
    for (int i = 0; i <= 360; i++)
    {
        float angle = i * 3.14159 / 180;
        float x = (cx + 50.0f) + cos(angle) * 8.0f;
        float y = -25.0f + sin(angle) * 8.0f;
        glVertex2f(x, y);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx - 50.0f, -25.0f);
    for (int i = 0; i <= 360; i++)
    {
        float angle = i * 3.14159 / 180;
        float x = (cx - 50.0f) + cos(angle) * 8.0f;
        float y = -25.0f + sin(angle) * 8.0f;
        glVertex2f(x, y);
    }
    glEnd();
    glPopMatrix();
}

void drawRedCar(float yOffset)
{
    glPushMatrix();
    glTranslatef(carX, -250.0f + yOffset, 0.0f);

    glBegin(GL_POLYGON);
    glColor3ub(255, 0, 0);
    glVertex2f(0, 0);
    glVertex2f(120, 0);
    glVertex2f(140, 25);
    glVertex2f(-20, 25);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(255, 0, 0);
    glVertex2f(20, 25);
    glVertex2f(100, 25);
    glVertex2f(90, 50);
    glVertex2f(30, 50);
    glEnd();

    glColor3ub(180, 220, 255);
    glBegin(GL_POLYGON);
    glVertex2f(25, 26);
    glVertex2f(55, 26);
    glVertex2f(55, 47);
    glVertex2f(33, 47);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(60, 26);
    glVertex2f(94, 26);
    glVertex2f(86, 47);
    glVertex2f(60, 47);
    glEnd();

    drawRotatingWheel(30.0f, 0.0f, 15.0f, 1);
    drawRotatingWheel(90.0f, 0.0f, 15.0f, 1);

    glPopMatrix();
}

void drawOrangeBus(float yOffset)
{

    glPushMatrix();
    glTranslatef(orangeBusPosition, yOffset, 0.0f);
    glBegin(GL_POLYGON);
    glColor3ub(255, 102, 0);
    glVertex2f(60.0f, -370.0f);
    glVertex2f(220.0f, -370.0f);
    glVertex2f(220.0f, -270.0f);
    glVertex2f(60.0f, -270.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(255, 255, 255);
    glVertex2f(220.0f, -360.0f);
    glVertex2f(270.0f, -360.0f);
    glVertex2f(250.0f, -305.0f);
    glVertex2f(220.0f, -305.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(0, 204, 204);
    glVertex2f(224.0f, -310.0f);
    glVertex2f(245.0f, -310.0f);
    glVertex2f(252.0f, -330.0f);
    glVertex2f(224.0f, -330.0f);
    glEnd();

    drawRotatingWheel(95.0f, -370.0f, 25.4f, 1);
    drawRotatingWheel(185.0f, -370.0f, 25.4f, 1);
    glPopMatrix();
}

void animateClouds(int value)
{
    cloudOffset -= 0.45f;
    if (cloudOffset < -650.0f)
    {
        cloudOffset = 650.0f;
    }
    glutPostRedisplay();
    glutTimerFunc(20, animateClouds, 0);
}

void animateOrangeBus(int value)
{
    if (orangeBusPosition > 600.0f)
        orangeBusPosition = -600.0f;

    orangeBusPosition += orangeBusSpeed;

    glutPostRedisplay();
    glutTimerFunc(20, animateOrangeBus, 0);
}

void animateBus(int value)
{
    busPosition -= 2.5f;
    if (busPosition < -600.0f)
        busPosition = 600.0f;
    wheelRotationAngle -= 10.0f;
    if (wheelRotationAngle < -360.0f)
        wheelRotationAngle += 360.0f;

    glutPostRedisplay();
    glutTimerFunc(16, animateBus, 0);
}

void animateRedCar(int value)
{
    carX += 2.0f;
    if (carX > 600)
    {
        carX = -350.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, animateRedCar, 0);
}

void animatePurpleCar(int value)
{
    purpleCarPosition -= purpleCarSpeed;

    if (purpleCarPosition < -600)
    {
        purpleCarPosition = 600.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, animatePurpleCar, 0);
}

void animateSailBoat(int value)
{
    // Ping-pong: boat travels between sailFarX and sailNearX.  When it
    // reaches either endpoint it reverses direction -- "first come to
    // near then turn and go to far".
    boat2X += 1.5f * (float)sailBoatDir;

    if (sailBoatDir == -1 && boat2X <= sailNearX)
    {
        boat2X = sailNearX;
        sailBoatDir = +1;             // turn, head back to far shore
    }
    else if (sailBoatDir == +1 && boat2X >= sailFarX)
    {
        boat2X = sailFarX;
        sailBoatDir = -1;             // turn, come near again
    }

    glutPostRedisplay();
    glutTimerFunc(50, animateSailBoat, 0);
}

void animateSteamBoatAndSmoke(int value)
{
    // Ping-pong motion for the steam boat as well.
    boatX += 1.5f * (float)steamBoatDir;

    if (steamBoatDir == +1 && boatX >= steamNearX)
    {
        boatX = steamNearX;
        steamBoatDir = -1;            // turn, head off to far
    }
    else if (steamBoatDir == -1 && boatX <= steamFarX)
    {
        boatX = steamFarX;
        steamBoatDir = +1;            // turn, come near
    }

    smokeAngle1 += 2.0f;
    smokeAngle2 += 2.0f;
    smokeAngle3 += 2.0f;

    smokeY1 += smokeSpeed;
    smokeY2 += smokeSpeed;
    smokeY3 += smokeSpeed;

    if (smokeY1 > 50.0f)
        smokeY1 = 0.0f;
    if (smokeY2 > 50.0f)
        smokeY2 = 0.0f;
    if (smokeY3 > 50.0f)
        smokeY3 = 0.0f;
    glutPostRedisplay();
    glutTimerFunc(20, animateSteamBoatAndSmoke, 0);
}

void animateWaves(int value)
{
    wavePhase += 0.07f;

    // Position -> scale:  t == 0 at the "near" endpoint (big boat),
    // t == 1 at the "far" endpoint (small boat).
    float sailT  = (boat2X - sailNearX) / (sailFarX - sailNearX);
    if (sailT < 0.0f) sailT = 0.0f;
    if (sailT > 1.0f) sailT = 1.0f;
    sailBoatPulse  = 1.5f - 1.0f * sailT;   // 1.5 near  ->  0.5 far

    float steamT = (boatX - steamNearX) / (steamFarX - steamNearX);
    if (steamT < 0.0f) steamT = 0.0f;
    if (steamT > 1.0f) steamT = 1.0f;
    steamBoatPulse = 1.5f - 1.0f * steamT;  // 1.5 near  ->  0.5 far

    glutPostRedisplay();
    glutTimerFunc(30, animateWaves, 0);
}

void animateRain(int value)
{
    if (rainOn)
    {
        for (int i = 0; i < RAIN_COUNT; i++)
        {
            rainY[i] -= rainSpeed[i];
            rainX[i] -= rainSpeed[i] * 0.3f;
            if (rainY[i] < -400.0f)
            {
                rainY[i] = 350.0f;
                rainX[i] = -400.0f + (rand() % 1000);
            }
            if (rainX[i] < -400.0f)
                rainX[i] = 600.0f;
        }
        glutPostRedisplay();
    }
    glutTimerFunc(20, animateRain, 0);
}

void handleMouseClick(int button, int state, int x, int y)
{
    if (state != GLUT_DOWN) return;

    // Left click  -> toggle rain
    // Right click -> toggle night mode
    if (button == GLUT_LEFT_BUTTON)
    {
        rainOn = !rainOn;
        glutPostRedisplay();
    }
    else if (button == GLUT_RIGHT_BUTTON)
    {
        nightOn = !nightOn;
        glutPostRedisplay();
    }
}

void initializeScene(void)
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(-400, 600, -400, 350);
    initRain();
    initStars();
}

void renderScenario()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawSky(0.0f);
    // Stars sit between the sky and the clouds so the clouds mask them.
    drawStars();
    drawClouds(0.0f);
    drawTallSkyTree(280, 0.0f);
    drawTallSkyTree(320, 0.0f);
    drawWaves();
    if (!nightOn)
    {
        drawSailBoat(boat2X, 100);
        drawSteamBoat(boatX, 50);
    }
    drawGrass(0.0f);
    drawPineTree(250, 0.0f);
    drawPineTree(300, 0.0f);
    drawLargeTree(-250, -10.0f);
    drawLargeTree(-190, -14.0f);
    drawMediumTree(-160, -6.0f);
    drawMediumTree(-250, -4.0f);
    drawMediumTree(150, -3.0f);
    drawMainRoad(0.0f);
    drawGrass(-150.0f);
    drawLowerRoad(0.0f);
    drawTenStoreyTower(-390.0f, -20.0f, 1.0f);
    drawSixStoreyBuilding(370.0f, -18.0f, 1.0f);
    drawTenStoreyTower(490.0f, -22.0f, 1.0f);
    drawLargeTree(-360, -25.0f);
    drawBus(0.0f);
    drawPurpleCar(0.0f);
    for (float x = -340; x <= 600; x += 70)
    {
        drawMedianTree(x, 10.0f);
    }
    for (int i = -400; i <= 600; i+= 200)
    {
        drawStreetLamp(i, 10.0f);
    }
    drawRedCar(-20.0f);
    drawOrangeBus(0.0f);
    drawNightOverlay();
    drawRain();
    glDisable(GL_DEPTH_TEST);
}

void displayScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderScenario();
    glutSwapBuffers();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1800, 1000);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Interactive Animated City & River Scene");
    initializeScene();
    glutDisplayFunc(displayScene);
    glutMouseFunc(handleMouseClick);
    glutTimerFunc(30, animateWaves, 0);
    glutTimerFunc(20, animateRain, 0);
    glutTimerFunc(20, animateClouds, 0);
    glutTimerFunc(20, animateOrangeBus, 0);
    glutTimerFunc(15, animateBus, 0);
    glutTimerFunc(0, animateRedCar, 0);
    glutTimerFunc(16, animatePurpleCar, 0);

    glutTimerFunc(50, animateSailBoat, 0);
    glutTimerFunc(20, animateSteamBoatAndSmoke, 0);

    glutMainLoop ();

   return 0; 
   
}