

#ifndef PNGLOADER_H
    #define PNGLOADER_H

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

    // libpng header
    #include <png.h>


    struct _glpngtexture {
        GLsizei  width;
        GLsizei  height;
        GLenum   format;
        GLint    internalFormat;
        GLuint   id;
        GLubyte *texels;
    };
    typedef struct _glpngtexture glpngtexture;


    glpngtexture *genPNGTexture(char *filename);
    void GetPNGtextureInfo (int color_type, glpngtexture *currentTexture);


    #ifdef __cplusplus
        }
    #endif

#endif
