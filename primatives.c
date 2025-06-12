
// OpenGL and GLUT headers
#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#endif

// Standard C headers
#include <stdio.h>
#include <math.h>

// Header for function prototypes
#include "primatives.h"

// Draw a trapezoidal side of a frustum (helper)
static void drawTrapezoid(GLdouble baseWidth, GLdouble topWidth, GLdouble height)
{
    GLdouble offset = (baseWidth - topWidth) / 2.0;

    glBegin(GL_QUADS);
        glNormal3f(0.0, 0.0, 1.0);
        glVertex3d(-baseWidth / 2.0, 0.0, 0.0);
        glVertex3d( baseWidth / 2.0, 0.0, 0.0);
        glVertex3d( baseWidth / 2.0 - offset, height, 0.0);
        glVertex3d(-baseWidth / 2.0 + offset, height, 0.0);
    glEnd();
}

// Draw a full frustum given base width, top width, and height
void drawFrustum(GLdouble baseWidth, GLdouble topWidth, GLdouble height)
{
    const int numFaces = 4;
    GLdouble offset     = (baseWidth - topWidth) / 2.0;
    GLdouble tiltAngle  = atan2(height, offset);
    GLdouble rotateDeg  = 90.0;
    GLdouble slantHeight = sqrt(offset * offset + height * height);
    GLdouble tiltDeg    = (M_PI_2 - tiltAngle) * (180.0 / M_PI);

    glMatrixMode(GL_MODELVIEW);

    for (int i = 0; i < numFaces; ++i) {
        glPushMatrix();
            glRotated(i * rotateDeg, 0.0, 1.0, 0.0);          // Rotate to face direction
            glTranslated(0.0, 0.0, baseWidth / 2.0);           // Move to face
            glRotated(-tiltDeg, 1.0, 0.0, 0.0);                // Tilt face
            drawTrapezoid(baseWidth, topWidth, slantHeight);  // Draw face
        glPopMatrix();
    }

    // Draw the top cap
    glBegin(GL_QUADS);
        glNormal3f(0.0, 1.0, 0.0);
        glVertex3d(-topWidth / 2.0, height,  topWidth / 2.0);
        glVertex3d( topWidth / 2.0, height,  topWidth / 2.0);
        glVertex3d( topWidth / 2.0, height, -topWidth / 2.0);
        glVertex3d(-topWidth / 2.0, height, -topWidth / 2.0);
    glEnd();
}
