/*
/*
 *  Draw primatives shapes in OpenGL
 */

#ifndef PRIMATIVES_H
    #define PRIMATIVES_H

    // make c++ friendly
    #ifdef __cplusplus
        extern "C" {
    #endif

    // OpenGL and GLUT headers
    #ifdef __APPLE__
        #include <GLUT/glut.h>
    #else
        #include <GL/gl.h>
        #include <GL/glu.h>
        #include <GL/glut.h>
    #endif

    // number of tessilations
    #define PRIMATIVE_RES 8

    // draw a frustum with base w1, top width w2, and height h
    void drawFrustum(GLdouble w1, GLdouble w2, GLdouble h);

    // draw a frustum with base w1, top width w2, and height h
    void drawTrap(GLdouble w1, GLdouble w2, GLdouble h);

    #ifdef __cplusplus
        }
    #endif

#endif
