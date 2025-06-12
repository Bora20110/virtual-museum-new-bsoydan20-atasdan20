

// standard c headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// OpenGL and GLUT headers
#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#endif

// type defs and prototypes
#include "navigator.h"

// debug level
short navDebug = NAV_DEBUG;

// window attributes
int windowID;
int winWidth  = DEFAULT_WIN_WIDTH;
int winHeight = DEFAULT_WIN_HEIGHT;

// current zoom
GLdouble zoomLevel = DEFAULT_ZOOM_LEVEL;

// camera variables
GLdouble cameraLocX = DEFAULT_CAMERA_X;
GLdouble cameraLocY = DEFAULT_CAMERA_Y;
GLdouble cameraLocZ = DEFAULT_CAMERA_Z;

GLdouble rotationH = DEFAULT_ROTATION_H;
GLdouble rotationV = DEFAULT_ROTATION_V;

//camera shake
bool cameraShaking = false;
int shakeDuration = 10;       // total frames
int shakeFrame = 0;
GLdouble shakeMagnitude = 300.0;

// clipping status
bool wallClipping = true;

// show the origin
bool showO = false;

// mouse variables
bool warpFlag = false;
int mouseMode = IDLE;

// keyboard variables
bool smoothMotionLeft  = false;
bool smoothMotionRight = false;
bool smoothMotionUp    = false;
bool smoothMotionDown  = false;
bool smoothMotionZoom  = false;
bool smoothMotionDuck  = false;
bool ducking           = false;
bool smoothMotionJump  = false;
bool jumping           = false;
bool smoothMotionOpen  = false;
GLdouble turnUnit      = DEFAULT_TURN_UNIT;
GLdouble moveUnit      = DEFAULT_MOVE_UNIT;
GLdouble jumpUnit      = DEFAULT_JUMP_UNIT;

// call-back functions
void (*navDraw)(void) = navDefaultDrawFunc;
void (*navClip)(GLdouble *x, GLdouble *y, GLdouble *z) = navDefaultClipFunc;
void (*navKey)(unsigned char key, int x, int y)   = navDefaultKeyFunc;
void (*navKeyUp)(unsigned char key, int x, int y) = navDefaultKeyUpFunc;

// initialize navigator
void navInit(int argc, char **argv)
{
    // Initialize GLUT with command-line arguments
    glutInit(&argc, argv);

    // Set up window, rendering settings, and callbacks
    navInitWindow(argc, argv);
    navInitDisplay();
    navInitCallBacks();
}

// initialize our window
void navInitWindow(int argc, char **argv)
{
    // Configure display mode: double-buffered RGB with depth, optional multisampling
    unsigned int displayMode = GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH;
    if (MULTISAMPLE_AA)
        displayMode |= GLUT_MULTISAMPLE;

    glutInitDisplayMode(displayMode);

    // Set initial window size and position
    glutInitWindowSize(winWidth, winHeight);
    glutInitWindowPosition(50, 50);

    // Create main application window
    windowID = glutCreateWindow("Interactive 3D Virtual Walkthrough of Museum");

    
}

// initialize our OpenGL display
void navInitDisplay()
{
    // Get current window dimensions
    winWidth  = glutGet(GLUT_WINDOW_WIDTH);
    winHeight = glutGet(GLUT_WINDOW_HEIGHT);

    // Configure perspective projection
    navWindowResize(winWidth, winHeight);

    // Set up camera modelview matrix
    navUpdateCamera();

    // Enable depth testing for correct z-buffer rendering
    glEnable(GL_DEPTH_TEST);

    // Enable smoothing for geometric primitives
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Enable multisample anti-aliasing (if supported)
    if (MULTISAMPLE_AA)
        glEnable(GL_MULTISAMPLE_ARB);

    // Set background color (black with full alpha)
    glClearColor(0.0, 0.0, 0.0, 1.0);

    // Configure alpha blending for transparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set polygon rendering mode (front faces filled)
    glPolygonMode(GL_FRONT, GL_FILL);

    // Enable back-face culling
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
}

// initialize mouse and keyboard
void navInitCallBacks()
{
    // --- Display and Window Events ---
    glutDisplayFunc(navDisplay);            // Redraw handler
    glutReshapeFunc(navWindowResize);       // Window resize handler

    // --- Cursor Style ---
    // Use invisible cursor for immersive experience
    glutSetCursor(GLUT_CURSOR_NONE);

    // --- Mouse Interaction ---
    glutMouseFunc(navMouse);                // Mouse click handler
    glutMotionFunc(navActiveMouse);         // Mouse drag (active motion) handler

    // --- Keyboard Input ---
    glutIgnoreKeyRepeat(true);              // Disable key repeat for clean input
    glutKeyboardFunc(navKeyboard);          // Key press handler
    glutKeyboardUpFunc(navKeyboardUp);      // Key release handler

    // --- Special Keys (arrows, function keys, etc.) ---
    glutSpecialFunc(navKeyboardArrow);      // Arrow/function key press
    glutSpecialUpFunc(navKeyboardArrowUp);  // Arrow/function key release
}

// register external display call-back
void navDrawFunc(void (*func)(void))
{
    navDraw = func;
}

void navDefaultDrawFunc()
{
    // insert super-kewl draw function here
}

// draw to the display
void navDisplay()
{
    // clear the display
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Update the camera view matrix based on current camera parameters
    navUpdateCamera();
    // Optionally draw the coordinate origin for debugging
    if (showO)
        navDrawOrigin();
    // Call the registered scene drawing function
    navDraw();

    // swap doubble buffers
    glutSwapBuffers();
}

// update our view of the world
void navUpdateCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (!CAMERA_UPDATE_MODE) {
        // Camera-centric mode: rotate camera around scene
        GLdouble lookX = cameraLocX - sin(rotationH * (M_PI / 180.0));
        GLdouble lookY = cameraLocY;
        GLdouble lookZ = cameraLocZ - cos(rotationH * (M_PI / 180.0));

        // --- Camera shake effect ---
        if (cameraShaking && shakeFrame < shakeDuration) {
            GLdouble offsetX = ((rand() % 100) / 100.0 - 0.5) * shakeMagnitude;
            GLdouble offsetY = ((rand() % 100) / 100.0 - 0.5) * shakeMagnitude;
            GLdouble offsetZ = ((rand() % 100) / 100.0 - 0.5) * shakeMagnitude;

            lookX += offsetX;
            lookY += offsetY;
            lookZ += offsetZ;

            shakeFrame++;
            if (shakeFrame >= shakeDuration) {
                cameraShaking = false;
            }
        }

        gluLookAt(cameraLocX, cameraLocY, cameraLocZ,
                  lookX, lookY, lookZ,
                  0.0, 1.0, 0.0);
    } else {
        // Scene-centric mode: rotate the world around camera
        glRotated(-rotationV, 1.0, 0.0, 0.0); // vertical rotation
        glRotated(-rotationH, 0.0, 1.0, 0.0); // horizontal rotation
        glTranslated(-cameraLocX, -cameraLocY, -cameraLocZ); // move scene
    }

    // Debug output for camera state
    if (navDebug > 0) {
        printf("cameraLocX: %.2f\n", cameraLocX);
        printf("cameraLocY: %.2f\n", cameraLocY);
        printf("cameraLocZ: %.2f\n", cameraLocZ);
        printf("rotationH:  %.2f\n", rotationH);
        printf("rotationV:  %.2f\n", rotationV);
    }
}

// draw the world origin in the scene
void navDrawOrigin()
{
    // save current modelview
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // draw x-axis
    glColor4d(1.0, 0.0, 0.0, 1.0);
    glBegin(GL_LINES);
        glVertex3d(0.0, 0.0, 0.0);
        glVertex3d(256.0, 0.0, 0.0);
    glEnd();

    // draw y-axis
    glColor4d(0.0, 1.0, 0.0, 1.0);
    glBegin(GL_LINES);
        glVertex3d(0.0, 0.0, 0.0);
        glVertex3d(0.0, 256.0, 0.0);
    glEnd();

    // draw z-axis
    glColor4d(0.0, 0.0, 1.0, 1.0);
    glBegin(GL_LINES);
        glVertex3d(0.0, 0.0, 0.0);
        glVertex3d(0.0, 0.0, 256.0);
    glEnd();

    // restore modelview
    glPopMatrix();
}

// respond to window resize
// reloads the perspective projection matrix
void navWindowResize(int newWidth, int newHeight)
{
    winWidth  = newWidth;
    winHeight = newHeight;

    double aspectRatio = (double)newWidth / (double)newHeight;

    // Switch to projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Setup perspective view volume using current zoom level
    GLdouble left   = -zoomLevel * aspectRatio;
    GLdouble right  =  zoomLevel * aspectRatio;
    GLdouble bottom = -zoomLevel;
    GLdouble top    =  zoomLevel;
    GLdouble near   = 512.0;
    GLdouble far    = 24000.0;

    glFrustum(left, right, bottom, top, near, far);

    // Define the viewport
    glViewport(0, 0, (GLsizei)newWidth, (GLsizei)newHeight);
}

void navClipFunc(void (*func)(GLdouble *x, GLdouble *y, GLdouble *z))
{
    navClip = func;
}

// default clipping function
// checks for floor clipping only
void navDefaultClipFunc(GLdouble *x, GLdouble *y, GLdouble *z)
{
    const GLdouble floorY = -650.0;
    const GLdouble minHeight = 420.0;
    const GLdouble floorLimit = floorY + minHeight;

    if (*y < floorLimit)
        *y = floorLimit;
}

// turn d degrees left
void navTurnHorizontal(GLdouble d)
{
    rotationH = fmod((rotationH + d), 360.0);
}

// turn d degrees up
void navTurnVertical(GLdouble delta)
{
    GLdouble newRotationV = rotationV + delta;

    if (newRotationV >= MAX_VERT_ROT) {
        rotationV = MAX_VERT_ROT;
    } else if (newRotationV <= -MAX_VERT_ROT) {
        rotationV = -MAX_VERT_ROT;
    } else {
        rotationV = fmod(newRotationV, 360.0);
    }
}

// move d units forward
void navMoveForward(GLdouble distance)
{
    GLdouble radianAngle = rotationH * (M_PI / 180.0);
    GLdouble deltaX = -distance * sin(radianAngle);
    GLdouble deltaZ = -distance * cos(radianAngle);

    cameraLocX += deltaX;
    cameraLocZ += deltaZ;

    if (wallClipping) {
        navClip(&cameraLocX, &cameraLocY, &cameraLocZ);
    }
}

// move d units sideways
void navMoveSideways(GLdouble distance)
{
    GLdouble radianAngle = rotationH * (M_PI / 180.0);
    GLdouble deltaX = -distance * cos(radianAngle);
    GLdouble deltaZ =  distance * sin(radianAngle);

    cameraLocX += deltaX;
    cameraLocZ += deltaZ;

    if (wallClipping) {
        navClip(&cameraLocX, &cameraLocY, &cameraLocZ);
    }
}

// move d units up
void navMoveUp(GLdouble distance)
{
    cameraLocY += distance;

    if (wallClipping)
        navClip(&cameraLocX, &cameraLocY, &cameraLocZ);
}

// zoom camera in or out
void navZoom(GLdouble deltaZoom)
{
    GLdouble aspectRatio = (GLdouble)winWidth / (GLdouble)winHeight;

    if ((zoomLevel - deltaZoom) <= 0.0) {
        zoomLevel = 0.1;
    } else if ((zoomLevel - deltaZoom) >= DEFAULT_ZOOM_LEVEL) {
        zoomLevel = DEFAULT_ZOOM_LEVEL;
    } else {
        zoomLevel -= deltaZoom;
    }

    // Update the perspective projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(
        -zoomLevel * aspectRatio, zoomLevel * aspectRatio,
        -zoomLevel, zoomLevel,
        512.0, 24000.0
    );
}

// register and external keyboard function
void navKeyboardFunc(void (*func)(unsigned char key, int x, int y))
{
    navKey = func;
}

// default keyboard call-back
void navDefaultKeyFunc(unsigned char key, int x, int y)
{
    if (key == 'q')
        exit(EXIT_SUCCESS);
}

// respond to key press
void navKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case '+':
        case '=':
            smoothMotionZoom = true;
            navSmoothMotion(ZOOM_IN);
            break;

        case '-':
        case '_':
            smoothMotionZoom = true;
            navSmoothMotion(ZOOM_OUT);
            break;

        case 'c':
            wallClipping = !wallClipping;
            break;

        case 'j':
        case 'J':
            if (!jumping) {
                jumping = true;
                smoothMotionJump = true;
                navSmoothMotion(JUMP);
            }
            break;

        case 'd':
        case 'D':
            smoothMotionDuck = true;
            navSmoothMotion(DUCK);
            break;

        case 'o':
            showO = !showO;
            glutPostRedisplay();
            break;
        

        case '0':
            // Reset view settings
            rotationV = DEFAULT_ROTATION_V;
            zoomLevel = DEFAULT_ZOOM_LEVEL;
            navZoom(0);
            glutPostRedisplay();
            break;

        default:
            break;
    }

    // Pass key to registered external function
    navKey(key, x, y);
}

// register and external key-release function
void navKeyboardUpFunc(void (*func)(unsigned char key, int x, int y))
{
    navKeyUp = func;
}

// default key release function
void navDefaultKeyUpFunc(unsigned char key, int x, int y)
{
    // no nothing
}

// respond to key release
void navKeyboardUp(unsigned char key, int x, int y)
{
    switch (key) {
        case '+':
        case '=':
        case '-':
        case '_':
            smoothMotionZoom = false;
            break;

        case 'u':
        case 'U':
            smoothMotionJump = false;
            break;

        case 'd':
        case 'D':
            smoothMotionDuck = false;
            break;

        default:
            break;
    }

    // Call user-defined key-up callback
    navKeyUp(key, x, y);
}

// respond to arrow press
void navKeyboardArrow(int key, int x, int y)
{
    int mod = glutGetModifiers();

    // Adjust turn and move speed with SHIFT
    turnUnit = (mod & GLUT_ACTIVE_SHIFT) ? DEFAULT_TURN_UNIT * 2.0 : DEFAULT_TURN_UNIT;
    moveUnit = (mod & GLUT_ACTIVE_SHIFT) ? DEFAULT_MOVE_UNIT * 2.0 : DEFAULT_MOVE_UNIT;

    switch (key) {
        case GLUT_KEY_LEFT:
            if (!smoothMotionLeft) {
                smoothMotionLeft = true;
                navSmoothMotion((mod & GLUT_ACTIVE_ALT) ? MOVE_LEFT : TURN_LEFT);
            }
            break;

        case GLUT_KEY_RIGHT:
            if (!smoothMotionRight) {
                smoothMotionRight = true;
                navSmoothMotion((mod & GLUT_ACTIVE_ALT) ? MOVE_RIGHT : TURN_RIGHT);
            }
            break;

        case GLUT_KEY_UP:
            if (!smoothMotionUp) {
                smoothMotionUp = true;
                navSmoothMotion((mod & GLUT_ACTIVE_ALT) ? TURN_UP : MOVE_FORWARD);
            }
            break;

        case GLUT_KEY_DOWN:
            if (!smoothMotionDown) {
                smoothMotionDown = true;
                navSmoothMotion((mod & GLUT_ACTIVE_ALT) ? TURN_DOWN : MOVE_BACKWARD);
            }
            break;

        default:
            break;
    }
}

// respond to arrow key release
void navKeyboardArrowUp(int key, int x, int y)
{
    switch (key) {
        case GLUT_KEY_LEFT:
            smoothMotionLeft = false;
            break;
        case GLUT_KEY_RIGHT:
            smoothMotionRight = false;
            break;
        case GLUT_KEY_UP:
            smoothMotionUp = false;
            break;
        case GLUT_KEY_DOWN:
            smoothMotionDown = false;
            break;
        default:
            break;
    }
}

// animate keyboard motion
void navSmoothMotion(int m)
{
    switch (m) {
        case MOVE_FORWARD:
            navMoveForward(moveUnit);
            if (smoothMotionUp)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case MOVE_BACKWARD:
            navMoveForward(-moveUnit);
            if (smoothMotionDown)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case MOVE_LEFT:
            navMoveSideways(moveUnit);
            if (smoothMotionLeft)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case MOVE_RIGHT:
            navMoveSideways(-moveUnit);
            if (smoothMotionRight)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case TURN_LEFT:
            navTurnHorizontal(turnUnit);
            if (smoothMotionLeft)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case TURN_RIGHT:
            navTurnHorizontal(-turnUnit);
            if (smoothMotionRight)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case TURN_UP:
            navTurnVertical(turnUnit);
            if (smoothMotionUp)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case TURN_DOWN:
            navTurnVertical(-turnUnit);
            if (smoothMotionDown)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case ZOOM_IN:
            navZoom(5.0);
            if (smoothMotionZoom)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case ZOOM_OUT:
            navZoom(-5.0);
            if (smoothMotionZoom)
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            break;

        case DUCK:
            if (smoothMotionDuck)
                navMoveUp(-70.0);
            else
                navMoveUp(70.0);

            if (smoothMotionDuck || (cameraLocY < 0.0))
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            else
                cameraLocY = 0.0;
            break;

        case JUMP:
            jumpUnit -= JUMP_DELTA;
            navMoveUp(jumpUnit);

            if (cameraLocY > 0.0) {
                glutTimerFunc(KEY_MOTION_DELAY, navSmoothMotion, m);
            } else {
                cameraLocY = 0.0;
                jumpUnit = DEFAULT_JUMP_UNIT;
                jumping = false;
            }
            break;
    }

    glutPostRedisplay();
}
// respond to mouse clicks
void navMouse(int button, int state, int x, int y)
{
    if (state == GLUT_UP) {
        mouseMode = IDLE;
        return;
    }

    switch (button) {
        case GLUT_LEFT_BUTTON:
            mouseMode = MOVING;
            break;
        case GLUT_MIDDLE_BUTTON:
            mouseMode = ZOOMING;
            break;
        case GLUT_RIGHT_BUTTON:
            mouseMode = TURNING;
            break;
    }

    warpFlag = true;
    glutWarpPointer(winWidth / 2, winHeight / 2);
}

// respond to mouse motion
void navActiveMouse(int x, int y)
{
    if (warpFlag) {
        warpFlag = false;  // Skip artificial event caused by glutWarpPointer
        return;
    }

    int centerX = winWidth / 2;
    int centerY = winHeight / 2;

    GLdouble dx = centerX - x;
    GLdouble dy = centerY - y;

    switch (mouseMode) {
        case TURNING:
            navTurnHorizontal(0.2 * dx);
            navTurnVertical(0.2 * dy);
            break;

        case ZOOMING:
            navZoom(dy);
            break;

        case MOVING:
            navMoveSideways(4.5 * dx);
            navMoveForward(6.0 * dy);
            break;

        case IDLE:
        default:
            return;
    }

    glutPostRedisplay();

    warpFlag = true;
    glutWarpPointer(centerX, centerY);
}
