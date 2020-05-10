// Created by: Garrett Miller, Josuha Annis
// Base file: fps.cpp
// date: Monday, Feb 3
//
//program: 3dDefYC.cpp
//author:  Gordon Griesel
//date:    Winter 2020
//
//Framework for a 3D game.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
//X11 utilities not currently needed.
//#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <fstream>
#include <iostream>
#include "time.h"
#include "defs.h"
#include "log.h"
#include "fonts.h"

typedef float Flt;
typedef Flt Vec[3];
typedef Flt Matrix[4][4];

using namespace std;

//some constants
const Vec upv = {0.0, 1.0, 0.0};
const int MAX_HEALTH = 35; 

//-----------------------------------------------------------------------------
//Setup timers
extern struct timespec timeStart, timeCurrent;
extern double timeDiff(struct timespec *start, struct timespec *end);
extern void timeCopy(struct timespec *dest, struct timespec *source);
//-----------------------------------------------------------------------------

void identity33(Matrix m);
void yy_transform(const Vec rotate, Matrix a);
void trans_vector(Matrix mat, const Vec in, Vec out);

class BlenderObject {
    float vert[4000][3];
    float normal[4000][3];
    int face[4000][3];
    int fnorm[4000][3];
    int nverts = 0;
    int nnorms = 0;
    int nfaces = 0;
    int has_normals;

    public:
    BlenderObject(const char *fname) {
        has_normals = 0;
        ifstream fin(fname);
        char line[100];
        fin.getline(line, 100);
        while (!fin.eof()) {
            //look for v
                if (*line == 'v') {
                    float pt[3];
                    sscanf(line+1, "%f %f %f", &pt[0], &pt[1], &pt[2]);
                    vert[nverts][0] = pt[0];
                    vert[nverts][1] = pt[1];
                    vert[nverts][2] = pt[2];
                    ++nverts;
                }
            //look for f
            if (*line == 'f') {
                int f[3];
                sscanf(line+1, "%i %i %i", &f[0], &f[1], &f[2]);
                face[nfaces][0] = f[0]-1;
                face[nfaces][1] = f[1]-1;
                face[nfaces][2] = f[2]-1;
                ++nfaces;
            }
            fin.getline(line, 100);
        }
        fin.close();
    }
    BlenderObject() {
        has_normals = 1;
        ifstream fin("StickFigure.obj");
        char line[100];
        fin.getline(line, 100);
        while (!fin.eof()) {
            //look for v
            if (*line == 'v') {
                float pt[3];
                sscanf(line+2, "%f %f %f", &pt[0], &pt[1], &pt[2]);
                vert[nverts][0] = pt[0];
                vert[nverts][1] = pt[1];
                vert[nverts][2] = pt[2];
                ++nverts;
            }
            //look for vn
            if (*line == 'v' && line[1] == 'n') {
                float pt[3];
                sscanf(line+3, "%f %f %f", &pt[0], &pt[1], &pt[2]);
                normal[nnorms][0] = pt[0];
                normal[nnorms][1] = pt[1];
                normal[nnorms][2] = pt[2];
                ++nnorms;
            }
            //look for f
            if (*line == 'f') {
                int f[3], n[3];
                sscanf(line+2, "%i//%i %i//%i %i//%i",
                        &f[0], &n[0], &f[1], &n[1], &f[2], &n[2]);
                face[nfaces][0] = f[0]-1;
                face[nfaces][1] = f[1]-1;
                face[nfaces][2] = f[2]-1;
                fnorm[nfaces][0] = n[0]-1;
                fnorm[nfaces][1] = n[1]-1;
                fnorm[nfaces][2] = n[2]-1;
                ++nfaces;
            }
            fin.getline(line, 100);
        }
        fin.close();
    }
    BlenderObject(const char *fname, int no) {
        has_normals = 1;
        ifstream fin(fname);
        char line[100];
        fin.getline(line, 100);
        while (!fin.eof()) {
            //look for v
            if (*line == 'v') {
                float pt[3];
                sscanf(line+2, "%f %f %f", &pt[0], &pt[1], &pt[2]);
                vert[nverts][0] = pt[0];
                vert[nverts][1] = pt[1];
                vert[nverts][2] = pt[2];
                ++nverts;
            }
            //look for vn
            if (*line == 'v' && line[1] == 'n') {
                float pt[3];
                sscanf(line+3, "%f %f %f", &pt[0], &pt[1], &pt[2]);
                normal[nnorms][0] = pt[0];
                normal[nnorms][1] = pt[1];
                normal[nnorms][2] = pt[2];
                ++nnorms;
            }
            //look for f
            if (*line == 'f') {
                int f[3], n[3];
                sscanf(line+2, "%i//%i %i//%i %i//%i",
                        &f[0], &n[0], &f[1], &n[1], &f[2], &n[2]);
                face[nfaces][0] = f[0]-1;
                face[nfaces][1] = f[1]-1;
                face[nfaces][2] = f[2]-1;
                fnorm[nfaces][0] = n[0]-1;
                fnorm[nfaces][1] = n[1]-1;
                fnorm[nfaces][2] = n[2]-1;
                ++nfaces;
            }
            fin.getline(line, 100);
        }
        fin.close();
    }
    void move(Flt x, Flt y, Flt z) {
        glTranslatef(x,y,z);
        draw();
    }
    void draw() {
        if (!has_normals) {
            glBegin(GL_TRIANGLES);
            for (int i=0; i<nfaces; i++) {
                for (int j=0; j<3; j++) {
                    glVertex3f(
                            vert[face[i][j]][0],
                            vert[face[i][j]][1],
                            vert[face[i][j]][2] );
                }
            }
            glEnd();
        }
        if (has_normals) {
            glBegin(GL_TRIANGLES);
            for (int i=0; i<nfaces; i++) {
                for (int j=0; j<3; j++) {
                    glNormal3f(
                            normal[fnorm[i][j]][0],
                            normal[fnorm[i][j]][1],
                            normal[fnorm[i][j]][2] );
                    glVertex3f(
                            vert[face[i][j]][0],
                            vert[face[i][j]][1],
                            vert[face[i][j]][2] );
                }
            }
            glEnd();
        }
    }
} castle("Castle.obj", 1), terrain("terrain.obj", 1);

class Pointer {
    public:
        Vec pos;
        Flt radius;
        Flt alpha;
        int nsegments;
    public:
        Pointer(Flt rad) {
            pos[0] = 0.0;
            pos[1] = 0.2;
            pos[2] = 50.0;
            nsegments = 20;
            radius = rad;
        }
        void translate(float x, float y, float z) {
            pos[0] += x;
            pos[1] += y;
            pos[2] += z;
        }
        int distanceToFigure(float x, float y, float z) {
            return sqrt(pow(pos[0]-x, 2) + pow(pos[1]-y, 2) + pow(pos[2]-z, 2));
        }
} outline(4.0), power(0.0);

class Camera {
    public:
        Vec position;
        Vec direction;
    public:
        Camera() {
            VecMake(80, 30, 30, position);
            VecMake(-8, -0.8, -1, direction);
        }
        void translate(float x, float y, float z) {
            position[0] += x;
            position[1] += y;
            position[2] += z;
        }
        void lookLeftRight(float angle) {
            Matrix mat;
            identity33(mat);
            Vec rot = {0.0, angle, 0.0};
            yy_transform(rot, mat);
            trans_vector(mat, direction, direction);
        }
        void lookUpDown(float angle) {
            void matrixFromAxisAngle(const Vec v, Flt ang, Matrix m);

            Vec rotate;
            Vec up = {0, 1, 0};
            VecCross(direction, up, rotate);

            Matrix mat;
            identity33(mat);

            matrixFromAxisAngle(rotate, angle, mat);

            trans_vector(mat, direction, direction);
        }
        void moveLeftRight(float dist) {
            //get perpendicular vector to direction/up
            Vec left;
            Vec up = {0,1,0};
            VecCross(direction, up, left); 
            position[0] += left[0] * dist;
            position[1] += left[1] * dist;
            position[2] += left[2] * dist;
        }
        void moveUpDown(float dist) {
            //just move straight up or down
            position[1] += dist;
        }

        void moveForwardBack(float dist) {
            position[0] += direction[0] * dist;
            position[1] += direction[1] * dist;
            position[2] += direction[2] * dist;
        }
};

//Class for a vector object.
class Myvec {
    public:
        Flt x, y, z;
        Myvec(Flt a, Flt b, Flt c) {
            x = a;
            y = b;
            z = c;
        }
        void make(Flt a, Flt b, Flt c) {
            x = a;
            y = b;
            z = c;
        }
        void negate() {
            x = -x;
            y = -y;
            z = -z;
        }
        void zero() {
            x = y = z = 0.0;
        }
        Flt dot(Myvec v) {
            return (x*v.x + y*v.y + z*v.z);
        }
        Flt lenNoSqrt() {
            return (x*x + y*y + z*z);
        }
        Flt len() {
            return sqrtf(lenNoSqrt());
        }
        void copy(Myvec b) {
            b.x = x;
            b.y = y;
            b.z = z;
        }
        void add(Myvec b) {
            x = x + b.x;
            y = y + b.y;
            z = z + b.z;
        }
        void sub(Myvec b) {
            x = x - b.x;
            y = y - b.y;
            z = z - b.z;
        }
        void scale(Flt s) {
            x *= s;
            y *= s;
            z *= s;
        }
        void addS(Myvec b, Flt s) {
            x = x + b.x * s;
            y = y + b.y * s;
            z = z + b.z * s;
        }
};

class Global {
    public:
        int xres, yres;
        Flt aspectRatio;
        Vec cameraPos;
        Flt cameraAngle;
        Flt moveInc;
        int powerMeter;
        int pDir;
        int pulseNum;
        bool figureAttacking;
        bool isInAir;
        Camera camera;
        GLfloat lightPosition[4];
        BlenderObject obj;
        Flt objHealth;
        double xDir;
        double yDir;
        bool isAttacking[10];
        int enemyAttacking;
        struct timespec healthStart[10];
        struct timespec healthTime;
        Flt health;
        Flt cameramat[4][4];
        Global() {
            //constructor
            moveInc = 0.0;
            xres=640;
            yres=480;
            aspectRatio = (GLfloat)xres / (GLfloat)yres;
            VecMake(0.0, 15.0, 75.0, cameraPos);
            cameraAngle = 0.0;
            powerMeter = 0;
            pDir = 0;
            pulseNum = 0;
            figureAttacking = 0;
            isInAir = 0;
            objHealth = 3.8f;
            health = MAX_HEALTH;
            enemyAttacking = 0;
            VecMake(100.0f, 240.0f, 40.0f, lightPosition);
            lightPosition[3] = 1.0f;
        }
        void init_opengl();
        void init();
        void check_mouse(XEvent *e);
        int check_keys(XEvent *e);
        bool onEnemy();
        void physics();
        void render();
} g;


class X11_wrapper {
    private:
        Display *dpy;
        Window win;
        GLXContext glc;
    public:
        X11_wrapper() {
            //Look here for information on XVisualInfo parameters.
            //http://www.talisman.org/opengl-1.1/Reference/glXChooseVisual.html
            Window root;
            GLint att[] = { GLX_RGBA,
                GLX_STENCIL_SIZE, 2,
                GLX_DEPTH_SIZE, 24,
                GLX_DOUBLEBUFFER, None };
            //GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
            //GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
            //XVisualInfo *vi;
            Colormap cmap;
            XSetWindowAttributes swa;
            setup_screen_res(640, 480);
            dpy = XOpenDisplay(NULL);
            if (dpy == NULL) {
                printf("\n\tcannot connect to X server\n\n");
                exit(EXIT_FAILURE);
            }
            root = DefaultRootWindow(dpy);
            XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
            if (vi == NULL) {
                printf("\n\tno appropriate visual found\n\n");
                exit(EXIT_FAILURE);
            } 
            cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
            swa.colormap = cmap;
            swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                StructureNotifyMask | SubstructureNotifyMask;
            win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
                    vi->depth, InputOutput, vi->visual,
                    CWColormap | CWEventMask, &swa);
            set_title();
            glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
            glXMakeCurrent(dpy, win, glc);
        }
        ~X11_wrapper() {
            XDestroyWindow(dpy, win);
            XCloseDisplay(dpy);
        }
        void setup_screen_res(const int w, const int h) {
            g.xres = w;
            g.yres = h;
            g.aspectRatio = (GLfloat)g.xres / (GLfloat)g.yres;
        }
        void check_resize(XEvent *e) {
            //The ConfigureNotify is sent by the
            //server if the window is resized.
            if (e->type != ConfigureNotify)
                return;
            XConfigureEvent xce = e->xconfigure;
            if (xce.width != g.xres || xce.height != g.yres) {
                //Window size did change.
                reshape_window(xce.width, xce.height);
            }
        }
        void reshape_window(int width, int height) {
            //window has been resized.
            setup_screen_res(width, height);
            //
            glViewport(0, 0, (GLint)width, (GLint)height);
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            glOrtho(0, g.xres, 0, g.yres, -1, 1);
            set_title();
        }
        void set_title() {
            //Set the window title bar.
            XMapWindow(dpy, win);
            XStoreName(dpy, win, "fps framework");
        }
        bool getXPending() {
            return XPending(dpy);
        }
        XEvent getXNextEvent() {
            XEvent e;
            XNextEvent(dpy, &e);
            return e;
        }
        void swapBuffers() {
            glXSwapBuffers(dpy, win);
        }
} x11;


int main()
{
    g.init_opengl();
    g.init();
    int done = 0;
    while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            g.check_mouse(&e);
            done = g.check_keys(&e);
        }
        g.physics();
        g.render();
        x11.swapBuffers();
    }
    //cleanup_fonts();
    return 0;
}

void Global::init() {
    srand(time(0)); 
    
    double randX = (rand() % 10);
    double randY = 30.0;
    g.xDir = (randX);
    g.yDir = (randY);
    for (int i=0; i<10; i++) {
        isAttacking[i] = false;
    }
}

void Global::init_opengl()
{
    //OpenGL initialization
    glClearColor(0.0f, 0.1f, 0.3f, 0.0f);
    //Enable surface rendering priority using a Z-buffer.
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    //Enable Phong shading of surfaces.
    glShadeModel(GL_SMOOTH);
    //
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, g.aspectRatio, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    //
    //Enable this so material colors are the same as vertex colors.
    glEnable(GL_COLOR_MATERIAL);
    //Enable diffuse lighting of surfaces with normals defined.
    glEnable(GL_LIGHTING);
    //Turn on a light
    glLightfv(GL_LIGHT0, GL_POSITION, g.lightPosition);
    glEnable(GL_LIGHT0);
    //Do this to allow fonts
    glEnable(GL_TEXTURE_2D);
    //initialize_fonts();
    //
    //Test the stencil buffer on this computer.
    //
    // https://www.opengl.org/discussion_boards/showthread.php/
    // 138452-stencil-buffer-works-on-one-machine-but-not-on-another
    //
    // Before you try using stencil buffer try this:
    // Code :
    // GLint stencilBits = 0;
    // glGetIntegerv(GL_STENCIL_BITS, &amp;stencilBits);
    // if (stencilBits < 1)
    //    MessageBox(NULL,"no stencil buffer.\n","Stencil test", MB_OK);
    GLint stencilBits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    if (stencilBits < 1) {
        printf("No stencil buffer on this computer.\n");
        printf("Exiting program.\n");
        exit(0);
    }
}

Flt vecNormalize(Vec vec) {
    Flt len = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
    if (len == 0.0) {
        VecMake(0.0,0.0,1.0,vec);
        return 1.0;
    }
    len = sqrt(len);
    Flt tlen = 1.0 / len;
    vec[0] *= tlen;
    vec[1] *= tlen;
    vec[2] *= tlen;
    return len;
}

bool Global::onEnemy() {
    if (powerMeter == 0) {
        double dist = sqrt(pow(xDir-outline.pos[0],2)+pow(yDir+10-outline.pos[2],2));
        printf("xDir: %f outline x: %f yDir: %f outline y: %f, dist: %f\n", xDir, outline.pos[0], yDir, outline.pos[2], dist);
        return (dist < 4.0);
    } else {
        return false;
    }
}

void Global::check_mouse(XEvent *e)
{
    //Did the mouse move?
    //Was a mouse button clicked?
    static int savex = 0;
    static int savey = 0;
    //
    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            //Left button is down
        }
        if (e->xbutton.button==3) {
            //Right button is down
        }
    }
    if (savex != e->xbutton.x || savey != e->xbutton.y) {
        //Mouse moved
        savex = e->xbutton.x;
        savey = e->xbutton.y;
    }
}

int Global::check_keys(XEvent *e)
{
    //Was there input from the keyboard?
    if (e->type == KeyPress) {
        int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
        switch(key) {
            case XK_Right:
                outline.translate(0.0,0.0,-1.0);

                break;
            case XK_Left:
                outline.translate(0.0,0.0,1.0);

                break;
            case XK_Up:
                outline.translate(-1.0,0.0,0.0);

                break;
            case XK_Down:
                outline.translate(1.0,0.0,0.0);
              
                break;
            case XK_space:
                powerMeter = !powerMeter;
                pulseNum = 0;
                if (onEnemy()) {
                    objHealth -= abs(power.radius);
                    printf("attack, health left %f\n", objHealth);
                    if (objHealth <= 0) {
                        objHealth = 3.8;
                        yDir = 40.0;
                        printf("\t\ndead, health reset to %f\n\n", objHealth);
                    }
                }
                power.radius = 0;

                break;
            case XK_a:
                enemyAttacking++;
                if(enemyAttacking > 9) {enemyAttacking--;}
                printf("enemy attacking: %d\n", enemyAttacking);
                isAttacking[enemyAttacking] = true;

                break;
            case XK_Escape:
                return 1;
        }
    }
    return 0;
}

void make_view_matrix(const Vec p1, const Vec p2, Matrix m)
{
    //Line between p1 and p2 form a LOS Line-of-sight.
    //A rotation matrix is built to transform objects to this LOS.
    //Diana Gruber  http://www.makegames.com/3Drotation/
    m[0][0]=m[1][1]=m[2][2]=1.0f;
    m[0][1]=m[0][2]=m[1][0]=m[1][2]=m[2][0]=m[2][1]=0.0f;
    Vec out = { p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2] };
    //
    Flt l1, len = out[0]*out[0] + out[1]*out[1] + out[2]*out[2];
    if (len == 0.0f) {
        VecMake(0.0f,0.0f,1.0f,out);
    } else {
        l1 = 1.0f / sqrtf(len);
        out[0] *= l1;
        out[1] *= l1;
        out[2] *= l1;
    }
    m[2][0] = out[0];
    m[2][1] = out[1];
    m[2][2] = out[2];
    Vec up = { -out[1] * out[0], upv[1] - out[1] * out[1], -out[1] * out[2] };
    //
    len = up[0]*up[0] + up[1]*up[1] + up[2]*up[2];
    if (len == 0.0f) {
        VecMake(0.0f,0.0f,1.0f,up);
    }
    else {
        l1 = 1.0f / sqrtf(len);
        up[0] *= l1;
        up[1] *= l1;
        up[2] *= l1;
    }
    m[1][0] = up[0];
    m[1][1] = up[1];
    m[1][2] = up[2];
    //make left vector.
    VecCross(up, out, m[0]);
}

void vecScale(Vec v, Flt s) 
{
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}

void drawPointer() {
    float theta = 2 * 3.1415926 / float(outline.nsegments); 
	float c = cosf(theta); //precalculate the sine and cosine
	float s = sinf(theta);
	float t;

	float x = outline.radius; //we start at angle = 0 
	float z = 0; 
    
	glBegin(GL_LINE_LOOP); 
	for(int ii = 0; ii < outline.nsegments; ii++) 
	{ 
		glVertex3f(x + outline.pos[0], outline.pos[1], z + outline.pos[2]);//output vertex 
        
		//apply the rotation matrix
		t = x;
		x = c * x - s * z;
		z = s * t + c * z;
	} 
	glEnd(); 
}
  
void drawHealth() {
    if(g.health >= 2*(MAX_HEALTH/3)){
        glColor3ub(0,255,0);
    }
    else if(g.health < 2*(MAX_HEALTH/3) && g.health > MAX_HEALTH/3) {
        glColor3ub(255,255,0);
    }
    else if(g.health <= MAX_HEALTH/3) {
        glColor3ub(255,0,0);
    }
    glPushMatrix();
    glRotatef(90, 0.0f, 1.0f, 0.0f);
    glRects(-20,40,g.health-20,45); //x1,y1,x2,y2
    glPopMatrix();
}

void drawFillPointer(Pointer pointer) {
    //static float angle;
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glLoadIdentity();
    // glTranslatef(0, 0, -10);
    double radius = pointer.radius;
    //glColor3ub(253, 184, 19);     
    glColor3ub(255, 0, 0);
    double twicePi = 2.0 * PI;
    glBegin(GL_TRIANGLE_FAN); //BEGIN CIRCLE
    glVertex2f(outline.pos[0], outline.pos[2]); // center of circle
    for (int i = 0; i <= outline.nsegments; i++)   {
        glVertex2f (
            (outline.pos[0] + (radius * cos(i * twicePi / 20))), (outline.pos[2] + (radius * sin(i * twicePi / 20)))
        );
    }
    glEnd(); //END
}

void drawPointer(Pointer pointer) {
    float theta = 2 * 3.1415926 / float(outline.nsegments); 
    float c = cosf(theta); //precalculate the sine and cosine
    float s = sinf(theta);
    float t;

    float x = pointer.radius; //we start at angle = 0 
    float z = 0; 
    
    glBegin(GL_LINE_LOOP); 
    for(int i = 0; i < outline.nsegments; i++) 
    { 
        glVertex2f(x + outline.pos[0], z + outline.pos[2]);//output vertex 
        
        //apply the rotation matrix
        t = x;
        x = c * x - s * z;
        z = s * t + c * z;
    } 
    glEnd(); 
}

void decreaseHealth(int enemy) {
        g.health -= 0.005;
        timeCopy(&g.healthStart[enemy], &g.healthTime);
}

void Global::physics()
{   
    clock_gettime(CLOCK_REALTIME, &g.healthTime);
    
    for (int i = 0; i < g.enemyAttacking+1; i++) {
        double d = timeDiff(&g.healthStart[i], &g.healthTime);

        if (d > 0.00001 && g.health > 0 && g.isAttacking[i])  {
            decreaseHealth(i);
        }
    }
    if (g.health > 0 && g.figureAttacking) {
        decreaseHealth(0);
    }
}

void drawObject(BlenderObject obj,Flt x, Flt y, Flt z) {
    glPushMatrix();
    
    glTranslatef(x,y,z);
    obj.draw();

    glPopMatrix();
}

void Global::render()
{
    Rect r;
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    //
    //3D mode
    //
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, g.aspectRatio, 0.5f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec up = { 0.0, 1.0, 0.0 };
    gluLookAt(
            g.camera.position[0], g.camera.position[1], g.camera.position[2],
            g.camera.position[0]+g.camera.direction[0],
            g.camera.position[1]+g.camera.direction[1],
            g.camera.position[2]+g.camera.direction[2],
            up[0], up[1], up[2]);
    
    drawHealth();

    glTranslatef(0.0,2,0.0);
    glRotatef(90,1.0,0.0,0.0);
    drawPointer(outline);
    drawFillPointer(power);
    glRotatef(-90,1.0,0.0,0.0);
    glTranslatef(0.0,-2,0.0);

    moveInc = 0.05;

    glColor3ub(100, 100, 100);
    drawObject(castle, 0.0,0.0,0.0);
    
    glColor3ub(20,255,20);  
    glScalef(4.0f,4.0f,5.0f);
    drawObject(terrain, 0.0,0.0,0.0);

    if (g.yDir > 6) {
        g.yDir -= g.moveInc;
    } else {
        g.figureAttacking = true;
    }

    glScalef(0.5f,0.5f,0.5f);

    glColor3ub(0, 0, 0);
    drawObject(g.obj,0,1.5,g.yDir);
    pulseNum++;
    
    if (powerMeter) {
        if (pulseNum == 100) {
            pDir = !pDir;
            printf("%d %d\n", pDir, pulseNum);
            pulseNum -= 100;
        }
        
        if (pDir == false) {
            power.radius += 0.04;
        } else {
            power.radius -= 0.04;
        }
    }

    //switch to 2D mode
    //
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
    glViewport(0, 0, g.xres, g.yres);
    glMatrixMode(GL_MODELVIEW);   glLoadIdentity();
    glMatrixMode (GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0, g.xres, 0, g.yres);
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;
    ggprint8b(&r, 16, 0x00FFFFFF, "A - Start additional castle attack");
    
    r.bot = g.yres - 40;
    ggprint8b(&r, 16, 0x00FFFFFF, "Space - Charge attack / Finish attack");
    
    r.bot = g.yres - 60;
    ggprint8b(&r, 16, 0x00FFFFFF, "Arrow keys - Move attack circle around");
    glPopAttrib();

}



