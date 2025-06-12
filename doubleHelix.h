
#ifndef DOUBLEHELIX_H
    #define DOUBLEHELIX_H

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

    // number of tesilations
    #define MOLI_RES 8
    #define BOND_RES 8

    // initialize draw routines
    void initDoubleHelix();

    // generate random material properties
    void genRandColor();

    // draw a sphere
    void drawMolicule(GLdouble tx, GLdouble ty, GLdouble tz, GLdouble rad);

    // draw a cylinder
    void drawBond(GLdouble tx, GLdouble ty, GLdouble tz,
                  GLdouble rr, GLdouble rx, GLdouble ry, GLdouble rz,
                  GLdouble rad, GLdouble h);

    // draw the double helix
    void drawDoubleHelix();

    #ifdef __cplusplus
        }
    #endif

#endif
