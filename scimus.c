
// standard c headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

// OpenGL and GLUT headers
#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#endif

// png loader library
#include "pngLoader.h"

// 3d navigation
#include "navigator.h"

// double helix
#include "doubleHelix.h"

// custom primative shapes
#include "primatives.h"

// frame cap
// removed for c compat, uncomment in animate as well
// #include "saveFrame.h"

// prototypes and macros
#include "scimus.h"

// debug level
short debug = DEBUG;

// textures and counts
glpngtexture *pix[MAX_NUM_PIX];
int           numPix;

bool showTextures = false;

// full screen mode status
bool gameMode = false;
int gameWindowID;

// status of screen cap
bool capture = false;

// amount window is open
bool glassIsOpening = false;
GLdouble glassOpen  = 0;

GLUquadric *quadric;

// animation variables
bool animation = false; // are we currently animating
bool frozen    = false; // is animation frozen
float speedMultiplier = 1.0f;  // default speed

//sound variables
bool soundPlayed = false;
bool playPourSound = false;
bool musicPlaying = false;
int musicPID = -1;


// sculpture1
GLdouble earthTheta = 0.0;
GLdouble earthDist  = 400.0;
GLdouble moonTheta  = 0.0;
GLdouble moonDist   = 75.0;
GLdouble mercuryTheta = 0.0;
GLdouble mercuryDist  = 300.0;

// sculpture2
GLdouble diskRot[4] = {0.0, 90.0, 0.0, 120.0};

// sculpture3
bool showHelix = true;
GLfloat teapotTiltAngle = 0.0f;
bool teapotPouringForward = true;

// sculpture4
GLdouble pistHeight = 0.0;
GLdouble crankTheta = 0.0;
GLdouble const crankRadius  = 210.0;
GLdouble const rodLength    = 300.0;
bool showBurn = false;



// main control loop
int main(int nargs, char *args[])
{
    // hard-coded texture file names
    char *p[2] = {
        "images/messi.png",
        "images/ceiling_texture.png",
    };

    // used for glu predefined shapes
    quadric = gluNewQuadric();
    gluQuadricOrientation(quadric, GLU_OUTSIDE);
    gluQuadricNormals(quadric, GLU_SMOOTH);

    // initialize double helix
    initDoubleHelix();

    // load pictures/textures from file
    loadTextures(2, p);

    // initialize the display window
    navInit(nargs, args);

    // initialize our pictures/textures 
    initTextures();

    // register glut call-backs 
    initCallBacks();

    // initialize scene lighting 
    initLighting();

    // pass control to glut 
    glutMainLoop();

    // all went well 
    return 0;
}

// load textures from file 
void loadTextures(int count, char *picNames[])
{
    int i;  // general use counter 

    // set our global number of textures 
    numPix = count;

    // check our array limit 
    if (numPix > MAX_NUM_PIX) {
        fprintf(stderr, "Fatal Error:  Attempted to Initialize %d textures.  Limit is %d.\n", numPix, MAX_NUM_PIX);
        exit(MAX_TEX_ERROR);
    }

    // initialize the array 
    for (i = 0; i < MAX_NUM_PIX; ++i)
        pix[i] = NULL;

    // for each texture 
    for (i = 0; i < numPix; ++i) {
        // use pngLoader to generate from png file 
        pix[i] = genPNGTexture(picNames[i]);
        printf("Loaded texture %d: %s (%dx%d)\n", i, picNames[i], pix[i]->width, pix[i]->height);

        // file dimentions must be a power of 2 or we're done 
        if ((!isPower2((pix[i])->width)) && (!isPower2((pix[i])->height))) {
            fprintf(stderr, "Fatal Error:  Invalid image size:  %dX%d.  "
            "Must be power of 2.", (pix[i]->width), (pix[i]->height));
            exit(IMAGE_SIZE_ERROR);
        }
    }
}

// generate OpenGL textures from the loaded images 
void initTextures()
{
    
    GLuint ids[MAX_NUM_PIX] = {0};  // array holding our texture id's


    // generate numPix identifiers and store them in ids 
    glGenTextures(numPix, ids);

    for (int i = 0; i < numPix; ++i) {
        // give each texture the id assigned by glGenTextures
        pix[i]->id = ids[i];

        // set texture properties and pixels 
        glBindTexture(GL_TEXTURE_2D, pix[i]->id);
        if (i == 0) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            gluBuild2DMipmaps(GL_TEXTURE_2D, pix[i]->internalFormat, pix[i]->width, pix[i]->height, pix[i]->format, GL_UNSIGNED_BYTE,pix[i]->texels);
            continue; // skip glTexImage2D below
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, pix[i]->internalFormat, pix[i]->width, pix[i]->height, 0, pix[i]->format, GL_UNSIGNED_BYTE, pix[i]->texels);
    }
}

// initialize glut call-backs 
void initCallBacks()
{
    // initialize display function 
    navDrawFunc(draw);

    // initialize keyboard functions 
    navKeyboardFunc(keyDown);
    navKeyboardUpFunc(keyUp);

    navClipFunc(enforceWallClipping);
}

// initialize scene lighting 
void initLighting() {
    GLfloat ambient[4] = {0.04, 0.04, 0.04, 1.0};

    GLfloat lightColors[8][3][4] = {
        {{0.20, 0.20, 0.01, 1.0}, {0.9, 0.9, 0.0, 1.0}, {0.9, 0.9, 0.0, 1.0}},
        {{0.25, 0.08, 0.01, 1.0}, {0.9, 0.2, 0.0, 1.0}, {0.9, 0.2, 0.0, 1.0}},
        {{0.90, 0.90, 0.90, 1.0}, {0.9, 0.9, 0.9, 1.0}, {0.0, 0.0, 0.0, 1.0}},
        {{0.90, 0.90, 0.90, 1.0}, {0.9, 0.9, 0.9, 1.0}, {0.0, 0.0, 0.0, 1.0}},
        {{0.20, 0.15, 0.15, 1.0}, {0.6, 0.6, 0.6, 1.0}, {0.6, 0.6, 0.6, 1.0}},
        {{0.20, 0.20, 0.20, 1.0}, {0.6, 0.6, 0.6, 1.0}, {0.6, 0.6, 0.6, 1.0}},
        {{0.20, 0.20, 0.20, 1.0}, {0.6, 0.6, 0.6, 1.0}, {0.6, 0.6, 0.6, 1.0}},
        {{0.20, 0.20, 0.20, 1.0}, {0.6, 0.6, 0.6, 1.0}, {0.6, 0.6, 0.6, 1.0}},
    };

    float constAtt[8]  = {0.5, 0.001, 2.2, 2.2, 0.001, 0.001, 0.001, 0.001};
    float linearAtt[8] = {0.0,  0.0001, 0.0001, 0.0001, 0.0001, 0.0001, 0.0001, 0.0001};
    float quadAtt[8]   = {0.0,  0.0000004, 0.0, 0.0, 0.0000005, 0.0000005, 0.0000005, 0.0000005};

    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    for (int i = 0; i < 8; ++i) {
        glEnable(GL_LIGHT0 + i);
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,  constAtt[i]);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,    linearAtt[i]);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, quadAtt[i]);
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  lightColors[i][0]);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  lightColors[i][1]);
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, lightColors[i][2]);
    }
}
// test if x is a power of 2

int isPower2(int x)
{
    return( (x > 0) && ((x & (x - 1)) == 0) );
}

void setMaterial(const GLfloat *ambient, const GLfloat *diffuse, const GLfloat *specular, GLfloat shininess)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  specular);
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

// draw to the display
void draw()
{
    // place lighting in the scene
    placeLights();

    // draw the floor
    drawFloor();

    // draw the ceiling
    drawCeiling();

    // draw the walls
    drawWalls();

    // draw the outside world
    drawOutside();

    drawSculpture1();
    drawSculpture2();
    drawSculpture3();
    drawSculpture4();
    drawSculpture5();

    // draw the window
    drawGlass();

    if (!animation && !frozen)
        animate(1);
    
}

// perform timed scene animation
void animate(int i)
{
    

    if (!frozen) {
        animation = true;
        updateSculpture1();
        updateSculpture2();
        updateSculpture3();
        updateSculpture4();
        openGlass();
        glutPostRedisplay();
        glutTimerFunc(ANI_RATE, animate, 1);
    }
    else
        animation = false;
}

// place lights in the scene
void placeLights()
{
    const GLfloat lightLocs[8][4] = {
         {0.0, 0.0, (ROOM_LENGTH / -2.0) + 0.0, 1.0},
         {0.0, 1024.0, (ROOM_LENGTH / -2.0) - 1024.0, 1.0},
         {OUTSIDE_WIDTH / -2.0 + 1024.0, (2.0 * FLOOR_LEVEL) + OUTSIDE_HEIGHT - 1024.0, (ROOM_LENGTH / -2.0) - OUTSIDE_LENGTH + 1024.0, 1.0},
         {OUTSIDE_WIDTH / 2.0 - 1024.0, (2.0 * FLOOR_LEVEL) + OUTSIDE_HEIGHT - 1024.0, (ROOM_LENGTH / -2.0) - OUTSIDE_LENGTH + 1024.0, 1.0},
         {(ROOM_WIDTH / 2.0) - 512, 512, (ROOM_LENGTH / 2.0) - (ROOM_LENGTH / 3.0), 1.0},
         {(ROOM_WIDTH / -2.0) + 512, 512, (ROOM_LENGTH / 2.0) - (ROOM_LENGTH / 3.0), 1.0},
         {(ROOM_WIDTH / 2.0) - 512, 512, (ROOM_LENGTH / 2.0) - (2.0 * ROOM_LENGTH / 3.0), 1.0},
         {(ROOM_WIDTH / -2.0) + 512, 512, (ROOM_LENGTH / 2.0) - (2.0 * ROOM_LENGTH / 3.0), 1.0}
     };

     const GLfloat spotdir0[3] = {0.0, -1.0, 2.5};

     for (int i = 0; i < 8; ++i) {
         glLightfv(GL_LIGHT0 + i, GL_POSITION, lightLocs[i]);
     }

     glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 65.0);
     glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 35.0);
     glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotdir0);
 }

// draw a tiled floor in the scene
void drawFloor()
{
    // Purple tile material
    const GLfloat materialSet1_A[4] = {0.3, 0.0, 0.4, 1.0};   // Ambient
    const GLfloat materialSet1_D[4] = {0.6, 0.1, 0.8, 1.0};   // Diffuse
    const GLfloat materialSet1_S[4] = {0.8, 0.5, 0.9, 1.0};   // Specular
    // Dark Blue tile material
    const GLfloat materialSet2_A[4] = {0.0, 0.0, 0.3, 1.0};   // Ambient
    const GLfloat materialSet2_D[4] = {0.1, 0.1, 0.6, 1.0};   // Diffuse
    const GLfloat materialSet2_S[4] = {0.2, 0.2, 0.8, 1.0};   // Specular

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
        glRotated(180.0, 1.0, 0.0, 0.0);
        glTranslated(-ROOM_WIDTH / 2.0, -FLOOR_LEVEL, -ROOM_LENGTH / 2.0);

        int tilesX = ROOM_WIDTH / 512;
        int tilesZ = ROOM_LENGTH / 512;

        for (int x = 0; x < tilesX; ++x) {
            for (int z = 0; z < tilesZ; ++z) {

                if (debug > 0) {
                    char debugLabel[16];
                    sprintf(debugLabel, "(%d,%d)", x, z);
                    drawText(x * 512, 0, z * 512, debugLabel);
                }

                if ((x + z) % 2 == 0) {
                    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   materialSet1_A);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   materialSet1_D);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  materialSet1_S);
                } else {
                    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   materialSet2_A);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   materialSet2_D);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  materialSet2_S);
                }

                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

                for (int i = x * 512 / TILE_RES; i < (x + 1) * 512 / TILE_RES; ++i) {
                    for (int j = z * 512 / TILE_RES; j < (z + 1) * 512 / TILE_RES; ++j) {
                        glBegin(GL_TRIANGLES);
                            glNormal3f(0.0, -1.0, 0.0);

                            // Define 1 triangle per tile — half of a square
                            glVertex3i(i * TILE_RES,       0, j * TILE_RES);
                            glVertex3i((i + 1) * TILE_RES, 0, j * TILE_RES);
                            glVertex3i(i * TILE_RES,       0, (j + 1) * TILE_RES);
                        glEnd();
                    }
                }
            }
        }
    glPopMatrix();
}

// draw a textured ceiling in the scene
void drawCeiling()
{
    const GLfloat ceilingMatA[4] = {0.5, 0.3, 0.6, 1.0};
    const GLfloat ceilingMatD[4] = {0.7, 0.4, 0.8, 1.0};
    const GLfloat ceilingMatS[4] = {0.1, 0.1, 0.1, 1.0};

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   ceilingMatA);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   ceilingMatD);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  ceilingMatS);
        glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

        glTranslated(-ROOM_WIDTH / 2.0, ROOM_HEIGHT + FLOOR_LEVEL, -ROOM_LENGTH / 2.0);

        int tilesX = ROOM_WIDTH / 512;
        int tilesZ = ROOM_LENGTH / 512;

        for (int x = 0; x < tilesX; ++x) {
            for (int z = 0; z < tilesZ; ++z) {
                if (showTextures)
                    glEnable(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, pix[numPix - 1]->id);

                glBegin(GL_QUADS);
                    glTexCoord2i(0, 0); glNormal3f(0.0, -1.0, 0.0); glVertex3i(x * 512,     0, z * 512);
                    glTexCoord2i(1, 0); glNormal3f(0.0, -1.0, 0.0); glVertex3i((x + 1) * 512, 0, z * 512);
                    glTexCoord2i(1, 1); glNormal3f(0.0, -1.0, 0.0); glVertex3i((x + 1) * 512, 0, (z + 1) * 512);
                    glTexCoord2i(0, 1); glNormal3f(0.0, -1.0, 0.0); glVertex3i(x * 512,     0, (z + 1) * 512);
                glEnd();

                if (showTextures)
                    glDisable(GL_TEXTURE_2D);
            }
        }
    glPopMatrix();
}

// draw walls in the scene
void drawWalls()
{
    int i, j;

    // material properties
    GLfloat const colorA[4] = {0.0, 0.0, 0.4, 1.0}; // Ambient
    GLfloat const colorD[4] = {0.0, 0.0, 0.6, 1.0}; // Diffuse
    GLfloat const colorS[4] = {0.0, 0.0, 0.8, 1.0}; // Specular
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   colorA);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   colorD);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  colorS);
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    // draw right wall
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    // rotate to face wall
    glRotated(-90.0, 0.0, 1.0, 0.0);
    // move to lower left corner
    glTranslated(ROOM_LENGTH/-2.0, FLOOR_LEVEL, ROOM_WIDTH/-2.0);
    // draw wall panels
    for (i = 0; i < ROOM_LENGTH/TILE_RES; ++i) {
        glBegin(GL_QUAD_STRIP);
        for(j = 0; j <= ROOM_HEIGHT/TILE_RES; ++j) {
            glNormal3f(0.0, 0.0, 1.0);
            glVertex3i( i   *TILE_RES, j*TILE_RES, 0);
            glNormal3f(0.0, 0.0, 1.0);
            glVertex3i((i+1)*TILE_RES, j*TILE_RES, 0);
        }
        glEnd();
    }
    glPopMatrix();

    // draw left wall
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // rotate to face wall
    glRotated(90.0, 0.0, 1.0, 0.0);

    // move to lower left corner
    glTranslated(ROOM_LENGTH/-2.0, FLOOR_LEVEL, ROOM_WIDTH/-2.0);

    // draw wall panels
    for (i = 0; i < ROOM_LENGTH/TILE_RES; ++i) {
        glBegin(GL_QUAD_STRIP);
        for(j = 0; j <= ROOM_HEIGHT/TILE_RES; ++j) {
            glNormal3f(0.0, 0.0, 1.0);
            glVertex3i( i   *TILE_RES, j*TILE_RES, 0);
            glNormal3f(0.0, 0.0, 1.0);
            glVertex3i((i+1)*TILE_RES, j*TILE_RES, 0);
        }
        glEnd();
    }
    glPopMatrix();

    // draw near wall
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // rotate to face wall
    glRotated(180.0, 0.0, 1.0, 0.0);

    // move to lower left corner
    glTranslated(ROOM_WIDTH/-2.0, FLOOR_LEVEL, ROOM_LENGTH/-2.0);

    // draw wall panels
    for (i = 0; i < ROOM_WIDTH/TILE_RES; ++i) {
        glBegin(GL_QUAD_STRIP);
        for (j = 0; j <= ROOM_HEIGHT/TILE_RES; ++j) {
            glNormal3f(0.0, 0.0, 1.0);
            glVertex3i( i   *TILE_RES, j*TILE_RES, 0);
            glNormal3f(0.0, 0.0, 1.0);
            glVertex3i((i+1)*TILE_RES, j*TILE_RES, 0);
        }
        glEnd();
    }
    glPopMatrix();

    // draw far wall
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // rotate to face wall
    // glRotated(0.0, 0.0, 1.0, 0.0);

    // move to lower left corner
    glTranslated(ROOM_WIDTH/-2.0, FLOOR_LEVEL, ROOM_LENGTH/-2.0);
    
    if (showTextures) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, pix[0]->id);  // skyline3.png
        printf("Binding messi texture ID %u on far wall\n", pix[0]->id);

        GLfloat const texColorA[4] = {1.0, 1.0, 1.0, 1.0};
        GLfloat const texColorD[4] = {1.0, 1.0, 1.0, 1.0};
        GLfloat const texColorS[4] = {1.0, 1.0, 1.0, 1.0};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, texColorA);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, texColorD);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, texColorS);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f,              0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(ROOM_WIDTH,        0.0f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(ROOM_WIDTH, ROOM_HEIGHT, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f,        ROOM_HEIGHT, 0.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    } else {
        // fallback: draw untextured wall
        for (int i = 0; i < ROOM_WIDTH / TILE_RES; ++i) {
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= ROOM_HEIGHT / TILE_RES; ++j) {
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i(i * TILE_RES, j * TILE_RES, 0);
                glVertex3i((i + 1) * TILE_RES, j * TILE_RES, 0);
            }
            glEnd();
        }
        
        // draw wall panels left of window
        for (i = 0; i < ((ROOM_WIDTH/TILE_RES/2)-(GLASS_WIDTH/TILE_RES/2)); ++i) {
            glBegin(GL_QUAD_STRIP);
            for (j = 0; j <= ROOM_HEIGHT/TILE_RES; ++j) {
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i( i   *TILE_RES, j*TILE_RES, 0);
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i((i+1)*TILE_RES, j*TILE_RES, 0);
            }
            glEnd();
        }
        
        // draw wall panels above window
        for (i = ((ROOM_WIDTH/TILE_RES/2)-(GLASS_WIDTH/TILE_RES/2));
             i < ((ROOM_WIDTH/TILE_RES/2)+(GLASS_WIDTH/TILE_RES/2)); ++i) {
            glBegin(GL_QUAD_STRIP);
            for (j = (GLASS_ELEV+GLASS_HEIGHT)/TILE_RES; j <= ROOM_HEIGHT/TILE_RES; ++j) {
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i( i   *TILE_RES, j*TILE_RES, 0);
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i((i+1)*TILE_RES, j*TILE_RES, 0);
            }
            glEnd();
        }
        
        // draw wall panels right of window
        for (i = ((ROOM_WIDTH/TILE_RES/2)+(GLASS_WIDTH/TILE_RES/2));
             i < ROOM_WIDTH/TILE_RES; ++i) {
            glBegin(GL_QUAD_STRIP);
            for (j = 0; j <= ROOM_HEIGHT/TILE_RES; ++j) {
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i( i   *TILE_RES, j*TILE_RES, 0);
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i((i+1)*TILE_RES, j*TILE_RES, 0);
            }
            glEnd();
        }
        
        // draw wall panels below window
        for (i = ((ROOM_WIDTH/TILE_RES/2)-(GLASS_WIDTH/TILE_RES/2));
             i < ((ROOM_WIDTH/TILE_RES/2)+(GLASS_WIDTH/TILE_RES/2)); ++i) {
            glBegin(GL_QUAD_STRIP);
            for (j = 0; j <= GLASS_ELEV/TILE_RES; ++j) {
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i( i   *TILE_RES, j*TILE_RES, 0);
                glNormal3f(0.0, 0.0, 1.0);
                glVertex3i((i+1)*TILE_RES, j*TILE_RES, 0);
            }
            glEnd();
        }
    }

    

    glPopMatrix();
}

// draw a glass window in the scene
void drawGlass()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslated(-GLASS_WIDTH / 2.0, FLOOR_LEVEL + GLASS_ELEV, ROOM_LENGTH / -2.0);

    // Frame Material
    GLfloat const frameA[] = {0.2, 0.2, 0.2, 1.0};
    GLfloat const frameD[] = {0.5, 0.5, 0.5, 1.0};
    GLfloat const frameS[] = {0.8, 0.8, 0.8, 1.0};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, frameA);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, frameD);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, frameS);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < 5; ++i) {
        float normalX = (i % 2 == 0) ? 1.0 : -1.0;
        float vertexX = (i < 2 || i == 4) ? 0 : GLASS_WIDTH;
        float vertexY = (i < 2) ? 0 : GLASS_HEIGHT;
        glNormal3f(normalX, (i < 2) ? 1.0 : -1.0, 0.0);
        glVertex3i(vertexX, vertexY, -50);
        glVertex3i(vertexX, vertexY, 0);
    }
    glEnd();

    // Glass Pane Material
    glDisable(GL_CULL_FACE);
    GLfloat const glassA[] = {0.1, 0.1, 0.7, 0.25};
    GLfloat const glassD[] = {0.1, 0.1, 0.7, 0.25};
    GLfloat const glassS[] = {0.1, 0.1, 0.7, 0.25};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, glassA);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, glassD);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, glassS);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    glBegin(GL_QUADS);
        glNormal3f(0.0, 0.0, 1.0);
        glVertex3d(0.0, 0.0, -50.0);
        glVertex3d(GLASS_WIDTH + glassOpen, 0.0, -50.0);
        glVertex3d(GLASS_WIDTH + glassOpen, GLASS_HEIGHT, -50.0);
        glVertex3d(0.0, GLASS_HEIGHT, -50.0);
    glEnd();
    glEnable(GL_CULL_FACE);

    glPopMatrix();
}

// update window animation
void openGlass()
{
    GLdouble delta = 50.0 * ANI_RATE / 200.0 * speedMultiplier;

    if (glassIsOpening) {
        glassOpen = fmax(glassOpen - delta, -GLASS_WIDTH);
    } else {
        glassOpen = fmin(glassOpen + delta, 0);
    }
}
void drawSculpture1()
{
    int i;

    // Material properties
    GLfloat coneA[]     = {0.33, 0.33, 0.33, 1.0};
    GLfloat coneD[]     = {0.78, 0.78, 0.78, 1.0};
    GLfloat coneS[]     = {0.90, 0.90, 0.90, 1.0};

    GLfloat sunA[]      = {0.6, 0.4, 0.1, 1.0};
    GLfloat sunD[]      = {0.8, 0.6, 0.1, 1.0};
    GLfloat sunS[]      = {1.0, 0.8, 0.1, 1.0};

    GLfloat earthA[]    = {0.1, 0.1, 0.4, 1.0};
    GLfloat earthD[]    = {0.1, 0.1, 0.6, 1.0};
    GLfloat earthS[]    = {0.1, 0.1, 0.8, 1.0};

    GLfloat moonA[]     = {0.3, 0.3, 0.3, 1.0};
    GLfloat moonD[]     = {0.8, 0.8, 0.8, 1.0};
    GLfloat moonS[]     = {0.0, 0.0, 0.0, 1.0};

    GLfloat mercuryA[]  = {0.3, 0.3, 0.3, 1.0};
    GLfloat mercuryD[]  = {0.8, 0.8, 0.8, 1.0};
    GLfloat mercuryS[]  = {0.0, 0.0, 0.0, 1.0};

    // Start drawing
    glPushMatrix();

    glTranslated((ROOM_WIDTH / 2.0) - 768.0, 0.0, (ROOM_LENGTH / 2.0) - (2.0 * ROOM_LENGTH / 5.0));

    // Stand base
    setMaterial(coneA, coneD, coneS, 27.8f);
    glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1.0, 0.0);
        glVertex3d(0.0, 0.0, 0.0);
        for (i = 0; i <= TILE_RES; ++i) {
            double angle = i * 2.0 * M_PI / TILE_RES;
            glNormal3f(sin(angle), 0.0, cos(angle));
            glVertex3d(100.0 * sin(angle), FLOOR_LEVEL, 100.0 * cos(angle));
        }
    glEnd();

    // Sun
    setMaterial(sunA, sunD, sunS, 100.0f);
    gluSphere(quadric, 128.0, 60, 40);
    glRotated(5.0, 0.0, 0.0, 1.0);  // Tilt for aesthetics

    // Earth + Moon
    glPushMatrix();
        glTranslated(earthDist * sin(earthTheta), 0.0, earthDist * -cos(earthTheta));
        setMaterial(earthA, earthD, earthS, 100.0f);
        gluSphere(quadric, 32.0, 35, 25);

        glTranslated(moonDist * sin(moonTheta), 0.0, moonDist * -cos(moonTheta));
        setMaterial(moonA, moonD, moonS, 1.0f);
        gluSphere(quadric, 10.0, 20, 15);
    glPopMatrix();

    // Mercury
    glPushMatrix();
        glTranslated(mercuryDist * sin(mercuryTheta), 0.0, mercuryDist * -cos(mercuryTheta));
        setMaterial(mercuryA, mercuryD, mercuryS, 1.0f);
        gluSphere(quadric, 20.0, 20, 15);
    glPopMatrix();

    glPopMatrix();
}





// update sculpture1 animation
void updateSculpture1()
{
    // Orbital constants
    const GLdouble EARTH_P = 350.0;
    const GLdouble EARTH_E = 0.75;

    const GLdouble MERCURY_P = 250.0;
    const GLdouble MERCURY_E = 0.58;

    GLdouble delta = (ANI_RATE / 200.0) * speedMultiplier;

    // Earth update
    GLdouble earthVelocity = (75000.0 / (earthDist * earthDist)) - (M_PI / 220.0);
    earthTheta += earthVelocity * delta;
    earthTheta  = fmod(earthTheta, 2.0 * M_PI);
    earthDist   = EARTH_P / (1 + EARTH_E * cos(earthTheta));

    // Moon update
    moonTheta += (M_PI / 6.0) * delta;
    moonTheta  = fmod(moonTheta, 2.0 * M_PI);

    // Mercury update
    GLdouble mercuryVelocity = (60000.0 / (mercuryDist * mercuryDist)) - (M_PI / 220.0);
    mercuryTheta += mercuryVelocity * delta;
    mercuryTheta  = fmod(mercuryTheta, 2.0 * M_PI);
    mercuryDist   = MERCURY_P / (1 + MERCURY_E * cos(mercuryTheta));
}

void drawSculpture2()
{
    const GLfloat colors[][4] = {
        {0.4, 0.2, 0.0, 1.0}, {0.8, 0.4, 0.0, 1.0}, {1.0, 0.5, 0.0, 1.0},    // Torus 1 - Orange
        {0.0, 0.3, 0.3, 1.0}, {0.0, 0.6, 0.6, 1.0}, {0.0, 0.9, 0.9, 1.0},    // Torus 2 - Teal
        {0.3, 0.0, 0.3, 1.0}, {0.6, 0.0, 0.6, 1.0}, {0.9, 0.0, 0.9, 1.0},    // Torus 3 - Magenta
        {0.3, 0.4, 0.0, 1.0}, {0.6, 0.8, 0.1, 1.0}, {0.9, 1.0, 0.3, 1.0},    // Torus 4 - Yellow-Green
        {0.3, 0.3, 0.3, 1.0}, {0.6, 0.6, 0.6, 1.0}, {0.8, 0.8, 0.8, 1.0}     // Support cylinders/spheres (neutral gray)
    };

    glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslated(-ROOM_WIDTH / 2.0 + 512, 0.0, ROOM_LENGTH / 2.0 - (2.0 * ROOM_LENGTH / 8.0));
        glRotated(90.0, 0.0, 1.0, 0.0);

        glPushMatrix();
        for (int i = 0; i < 4; ++i) {
            glRotated(diskRot[i], (i % 2 == 0), (i % 2 == 1), 0.0);

            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   colors[i * 3 + 0]);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   colors[i * 3 + 1]);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  colors[i * 3 + 2]);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

            glDisable(GL_CULL_FACE);
            glutSolidTorus(10.0, 210.0 - 20 * i, 20, 50);
            glEnable(GL_CULL_FACE);
        }
        glPopMatrix();

        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslated(side * 230.0, 0.0, 0.0);
            glRotated(90.0, 1.0, 0.0, 0.0);

            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   colors[12]);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   colors[13]);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  colors[14]);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

            gluCylinder(quadric, 10.0, 10.0, -1.0 * FLOOR_LEVEL, 20, 80);
            gluSphere(quadric, 10.0, 10, 15);
            glPopMatrix();
        }

        glPopMatrix();
    }

// update sculpture2 animation
void updateSculpture2() {
    const GLfloat rotationSpeeds[] = {5.0, 15.0, 25.0, 35.0};
    GLfloat delta = (ANI_RATE / 200.0) * speedMultiplier;

    for (int i = 0; i < 4; ++i) {
        diskRot[i] += rotationSpeeds[i] * delta;
        diskRot[i] = fmod(diskRot[i], 360.0);
    }
}

void drawSculpture3() {
    const GLfloat ambient[]  = {0.33, 0.22, 0.03, 1.0};
    const GLfloat diffuse[]  = {0.78, 0.57, 0.11, 1.0};
    const GLfloat specular[] = {0.99, 0.91, 0.81, 1.0};
    const GLfloat shininess  = 100.0f;

    // Set material properties for entire sculpture
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  specular);
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

        // Position sculpture in scene
        GLdouble x = -ROOM_WIDTH / 2.0 + 512;
        GLdouble z = ROOM_LENGTH / 2.0 - 4.0 * ROOM_LENGTH / 8.0;
        glTranslated(x, 0.0, z);

        // Draw base stand
        glPushMatrix();
        glTranslated(0.0, FLOOR_LEVEL, 0.0);
        drawFrustum(512.0, 128.0, 512.0);
        glPopMatrix();

        // Rotate and draw teapot
        glRotated(90.0, 0.0, 1.0, 0.0);  // keep this for orientation
        glRotated(-teapotTiltAngle, 0.0, 0.0, 1.0); // tilt forward/backward
        glDisable(GL_CULL_FACE);
        glutSolidTeapot(128.0);
        glEnable(GL_CULL_FACE);

    glPopMatrix();
}

void updateSculpture3()
{
    const float tiltSpeed = 2.0f;
    const float maxTilt = 45.0f;

    if (teapotPouringForward) {
        teapotTiltAngle += tiltSpeed;

        // 🔊 Only play sound if user enabled it with 'p'
        if (playPourSound && !soundPlayed && teapotTiltAngle >= tiltSpeed) {
            system("afplay pour.wav &");  // async sound (macOS)
            soundPlayed = true;
        }

        if (teapotTiltAngle >= maxTilt) {
            teapotTiltAngle = maxTilt;
            teapotPouringForward = false;
        }
    } else {
        teapotTiltAngle -= tiltSpeed;
        if (teapotTiltAngle <= 0.0f) {
            teapotTiltAngle = 0.0f;
            teapotPouringForward = true;
            soundPlayed = false;  // reset for next pour
        }
    }
}


void drawSculpture4()
{
    // --- Define Material Properties ---
    const GLfloat metalAmbient[]  = {0.4, 0.4, 0.4, 1.0};
    const GLfloat metalDiffuse[]  = {0.6, 0.6, 0.6, 1.0};
    const GLfloat metalSpecular[] = {0.6, 0.6, 0.6, 1.0};

    const GLfloat blockAmbient[]  = {0.4, 0.4, 0.4, 0.30};
    const GLfloat blockDiffuse[]  = {0.4, 0.4, 0.4, 0.30};
    const GLfloat blockSpecular[] = {1.0, 1.0, 1.0, 0.30};

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // --- Position the Sculpture ---
    glTranslated((ROOM_WIDTH / 2.0) - 512, 200.0, (ROOM_LENGTH / 2.0) - (3.0 * ROOM_LENGTH / 5.0));

    // --- Explosion Effect (Currently Disabled) ---
    if (0) {
        glPushMatrix();

        const GLfloat redA[] = {0.4, 0.0, 0.0, 1.0};
        const GLfloat redD[] = {0.8, 0.0, 0.0, 1.0};
        const GLfloat redS[] = {0.9, 0.0, 0.0, 1.0};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, redA);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, redD);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, redS);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

        glTranslated(0.0, -rodLength - 420.0, 0.0);

        if (fabs(crankTheta) < (40.0 * M_PI / 180.0))
            glScaled(1.0, 200.0 / pistHeight + 0.1, 1.0);
        else
            glScaled(1.0, 0.9 - 200.0 / pistHeight + 0.1, 1.0);

        gluSphere(quadric, 256.0, 20, 30);
        glPopMatrix();
    }

    // --- Metallic Look for Piston Assembly ---
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, metalAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, metalDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, metalSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    // --- Draw Main Piston Assembly ---
    glPushMatrix();

    // Crank Shaft Wall Mount
    glPushMatrix();
        glTranslated(150.0, 0.0, 0.0);
        glRotated(90.0, 0.0, 1.0, 0.0);
        gluSphere(quadric, 50.0, 20, 30);
        gluCylinder(quadric, 50.0, 50.0, 362.0, 20, 30);
    glPopMatrix();

    // Rotating Crank
    glPushMatrix();
        glTranslated(150.0, 0.0, 0.0);
        glRotated(-crankTheta * 180.0 / M_PI + 90.0, 1.0, 0.0, 0.0);
        gluCylinder(quadric, 50.0, 50.0, crankRadius, 20, 30);
        glTranslated(0.0, 0.0, crankRadius);
        gluSphere(quadric, 50.0, 20, 30);
    glPopMatrix();

    // Apply piston height and orientation
    glTranslated(0.0, -pistHeight, 0.0);
    glRotated(90.0, 1.0, 0.0, 0.0);

    // Main piston
    gluCylinder(quadric, 256.0, 256.0, 128.0, 20, 30);

    // Top of piston
    glRotated(180.0, 1.0, 0.0, 0.0);
    gluDisk(quadric, 0.0, 256.0, 20, 30);

    // Push Rod Mechanism
    glPushMatrix();
        glRotated(asin(crankRadius * sin(crankTheta) / rodLength) * 180.0 / M_PI, 1.0, 0.0, 0.0);
        gluSphere(quadric, 50.0, 20, 30);
        gluCylinder(quadric, 50.0, 50.0, rodLength, 20, 30);

        // Joint to crankshaft
        glTranslated(0.0, 0.0, rodLength);
        glRotated(90.0, 0.0, 1.0, 0.0);
        gluSphere(quadric, 50.0, 20, 30);
        gluCylinder(quadric, 50.0, 50.0, 150.0, 20, 30);
    glPopMatrix();

    // Bottom cap of piston
    glTranslated(0.0, 0.0, -128.0);
    glRotated(-180.0, 1.0, 0.0, 0.0);
    gluDisk(quadric, 0.0, 256.0, 20, 30);

    glPopMatrix(); // End of piston assembly

    // --- Draw Transparent Block Enclosure ---
    glPushMatrix();
        glTranslated(0.0, FLOOR_LEVEL - 200, 0.0);
        glRotated(-90.0, 1.0, 0.0, 0.0);

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, blockAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, blockDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, blockSpecular);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

        glDisable(GL_CULL_FACE);
        gluCylinder(quadric, 260.0, 260.0, 670.0, 60, 80);
        glEnable(GL_CULL_FACE);
    glPopMatrix();

    glPopMatrix(); // End of sculpture
}

void updateSculpture4()
{
    // Increment crank angle based on animation rate and speed multiplier
    crankTheta += (35.0 * (M_PI / 180.0)) * (ANI_RATE / 200.0) * speedMultiplier;
    crankTheta = fmod(crankTheta, 2.0 * M_PI);

    // Enable burn effect if crank is near vertical
    showBurn = fabs(crankTheta) < (90.0 * (M_PI / 180.0));

    // Update piston height using trigonometric crank-rod relationship
    double sinTheta = sin(crankTheta);
    double cosTheta = cos(crankTheta);
    pistHeight = crankRadius * cosTheta + sqrt(rodLength * rodLength - crankRadius * crankRadius * sinTheta * sinTheta);
}

// draw sculpture5
void drawSculpture5()
{
    if (!showHelix) return;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

        // Position the sculpture in the scene
        glTranslated(
            (ROOM_WIDTH / -2.0) + 512.0,
            0.0,
            (ROOM_LENGTH / 2.0) - (6.0 * ROOM_LENGTH / 8.0)
        );

        // Rotate and scale the helix
        glRotated(-95.0, 1.0, 0.0, 0.0);
        glScaled(35.0, 35.0, 35.0);

        // Draw the actual structure
        drawDoubleHelix();

    glPopMatrix();
}



// draw everything outside the room
void drawOutside()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Position the outside area
    glTranslated(
        OUTSIDE_WIDTH / -2.0,
        2.0 * FLOOR_LEVEL,
        ROOM_LENGTH / -2.0
    );

    // ----- Draw Grass (green base) -----
    {
        GLfloat grassAmbient[]  = {0.0, 1.0, 0.0, 1.0};
        GLfloat grassDiffuse[]  = {0.0, 1.0, 0.0, 1.0};
        GLfloat grassSpecular[] = {0.0, 0.0, 0.0, 1.0};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  grassAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  grassDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, grassSpecular);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

        glBegin(GL_QUADS);
            glVertex3i(0,             0,  0);
            glVertex3i(OUTSIDE_WIDTH, 0,  0);
            glVertex3i(OUTSIDE_WIDTH, 0, -OUTSIDE_LENGTH);
            glVertex3i(0,             0, -OUTSIDE_LENGTH);
        glEnd();
    }

    // ----- Draw Skyline Texture or Fallback -----
    if (showTextures)
    {
        printf("Inside drawOutside(): binding ceiling texture ID %u\n", pix[0]->id);

        GLfloat skyAmbient[]  = {1.0, 1.0, 1.0, 1.0};
        GLfloat skyDiffuse[]  = {1.0, 1.0, 1.0, 1.0};
        GLfloat skySpecular[] = {1.0, 1.0, 1.0, 1.0};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  skyAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  skyDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, skySpecular);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, pix[0]->id);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3i(0,             0,              -OUTSIDE_LENGTH);
            glTexCoord2f(1.0f, 0.0f); glVertex3i(OUTSIDE_WIDTH, 0,              -OUTSIDE_LENGTH);
            glTexCoord2f(1.0f, 1.0f); glVertex3i(OUTSIDE_WIDTH, OUTSIDE_HEIGHT, -OUTSIDE_LENGTH);
            glTexCoord2f(0.0f, 1.0f); glVertex3i(0,             OUTSIDE_HEIGHT, -OUTSIDE_LENGTH);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        // Fallback: Solid red wall if textures are disabled
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0, 0.0, 0.0);

        glBegin(GL_QUADS);
            glVertex3i(0,             0,              -OUTSIDE_LENGTH);
            glVertex3i(OUTSIDE_WIDTH, 0,              -OUTSIDE_LENGTH);
            glVertex3i(OUTSIDE_WIDTH, OUTSIDE_HEIGHT, -OUTSIDE_LENGTH);
            glVertex3i(0,             OUTSIDE_HEIGHT, -OUTSIDE_LENGTH);
        glEnd();
    }

    glPopMatrix();
}

// draw text at x, y, z
// doesn't work well in 3d
// need to rotate text to face camera
void drawText(int x, int y, int z, char *text)
{
    if (text == NULL || text[0] == '\0') return;

    glColor4d(0.0, 0.0, 0.0, 1.0);  // Set text color to black

    for (int i = 0; text[i] != '\0'; ++i) {
        glRasterPos3i(x, y, z);
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);
        x += glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);
    }
}

// respond to key press
void keyDown(unsigned char key, int x, int y)
{
    GLenum lights[] = {
        GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
        GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7
    };

    if (isdigit(key)) {
        int keyDigit = key - '0';
        if (keyDigit >= 1 && keyDigit <= 8) {
            int index = keyDigit - 1;
            if (glIsEnabled(lights[index]))
                glDisable(lights[index]);
            else
                glEnable(lights[index]);

            glutPostRedisplay();
        }
        return;
    }

    switch (key) {
        case 'a':
            frozen = !frozen;
            glutPostRedisplay();
            break;

        case 'f':
            frozen = true;
            if (!gameMode) {
                if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) {
                    gameWindowID = glutGetWindow();
                    glutEnterGameMode();

                    navInitDisplay();
                    navInitCallBacks();
                    initCallBacks();
                    initLighting();
                    initTextures();

                    gameMode = true;
                } else {
                    fprintf(stderr, "Full screen mode is not available.\n");
                }
            } else {
                glutLeaveGameMode();
                glutSetWindow(gameWindowID);

                navInitDisplay();
                navInitCallBacks();
                initCallBacks();
                initLighting();
                initTextures();

                gameMode = false;
            }
            frozen = false;
            break;

        case 'h':
            showHelix = !showHelix;
            glutPostRedisplay();
            break;

        case 'k':
            capture = !capture;
            break;

        case 'q':
            cleanUpAndQuit();
            break;

        case 't':
            showTextures = !showTextures;
            glutPostRedisplay();
            break;
            
        case 'm':
            if (!musicPlaying) {
                musicPID = fork();
                if (musicPID == 0) {
                    // Child process: play music
                    execlp("afplay", "afplay", "background.wav", (char *)NULL);
                    exit(1); // If execlp fails
                }
                musicPlaying = true;
            } else {
                if (musicPID > 0) {
                    kill(musicPID, SIGKILL);
                    musicPID = -1;
                }
                musicPlaying = false;
            }
            break;
        case 'i':
            printf("\n=== Museum Summary ===\n");
            printf("• Sculpture 1: Mini Solar System model with Sun, Earth, Moon, Mercury.\n");
            printf("• Sculpture 2: Rotating multi-colored torus disks.\n");
            printf("• Sculpture 3: Golden teapot tilting animation.\n");
            printf("• Sculpture 4: Mechanical piston and crank assembly.\n");
            printf("• Sculpture 5: DNA double helix structure.\n");
            printf("• Press 't': Image on wall and ceiling\n");
            printf("• Press '1-8': Lighting of the museum.\n");
            printf("• Press 'Arrow Keys': Move in the museum.\n");
            printf("• Press 'd': Little up and down movement.\n");
            printf("• Press 'i': Information about the museum.\n");
            printf("• Press 'p': Play teapot pouring sound.\n");
            printf("• Press 's': Double animation speed.\n");
            printf("• Press 'a': Freeze/unfreeze animations.\n");
            printf("• Press 'm': Music for museum.\n");
            printf("• Press 'q': Quitting the museum.\n");
            printf("• Press 'h': Show/Disappear Sculptıre 5.\n");
            printf("• Press '+': Zoom in.\n");
            printf("• Press '-': Zoom out.\n");
            printf("• Press 'j': Jump in the museum.\n");
            printf("=======================\n\n");
            break;

        case 'w':
            glassIsOpening = !glassIsOpening;
            break;
        case 's':
            speedMultiplier = (speedMultiplier == 1.0) ? 2.0 : 1.0;
            break;
        case 'p':
            playPourSound = !playPourSound;
            break;
        case 'e':
            printf("DEBUG: 'e' key pressed — starting camera shake.\n");
            fflush(stdout);  // ensures output is printed immediately
            cameraShaking = true;
            shakeFrame = 0;
            glutPostRedisplay();  // force redraw for instant feedback
            break;
        default:
            break;
    }
}
// respond to key release
void keyUp(unsigned char key, int x, int y)
{
    // This function is intentionally left empty.
    // Add key release behavior here if needed in the future.
}

// don't allow the camera to go outside the walls
void enforceWallClipping(GLdouble *x, GLdouble *y, GLdouble *z)
{
    GLdouble xMax = (ROOM_WIDTH  / 2.0) - 512.0 - WALL_CLIP_H;
    GLdouble xMin = (ROOM_WIDTH  / -2.0) + 512.0 + WALL_CLIP_H;
    GLdouble zMax = (ROOM_LENGTH / 2.0) - 512.0 - WALL_CLIP_H;
    GLdouble zMin = (ROOM_LENGTH / -2.0) + 512.0 + WALL_CLIP_H;
    GLdouble yMax = ROOM_HEIGHT + FLOOR_LEVEL - WALL_CLIP_V;
    GLdouble yMin = FLOOR_LEVEL + WALL_CLIP_V;

    if (*x > xMax) *x = xMax;
    if (*x < xMin) *x = xMin;

    if (*z > zMax) *z = zMax;
    if (*z < zMin) *z = zMin;

    if (*y > yMax) *y = yMax;
    if (*y < yMin) *y = yMin;
}

// clean up and exit
void cleanUpAndQuit()
{
    // Release allocated memory for loaded textures
    for (int i = 0; i < numPix; ++i) {
        if (pix[i] != NULL) {
            free(pix[i]);
            pix[i] = NULL;
        }
    }

    // Exit the program successfully
    exit(ALL_IS_WELL);
}
