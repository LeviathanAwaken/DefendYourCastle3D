// modified by: Garrett Miller
// date: Monday, Feb 3
//
//program: Castle Defender 3D.cpp
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
const int MAX_SMOKES = 400;
const int MAX_HEALTH = 35; 

//-----------------------------------------------------------------------------
//Setup timers
const double OOBILLION = 1.0 / 1e9;
extern struct timespec timeStart, timeCurrent;
extern double timeDiff(struct timespec *start, struct timespec *end);
extern void timeCopy(struct timespec *dest, struct timespec *source);
//-----------------------------------------------------------------------------

void identity33(Matrix m);
void yy_transform(const Vec rotate, Matrix a);
void trans_vector(Matrix mat, const Vec in, Vec out);

class movementRecorder {
    public:
        Vec position[100];
        Vec direction[100];
    public:
        void saveScript(const char *fname) {
            ofstream fout(fname);
            for (int i=0; i<100; i++) {
                fout << position;
            }   
            fout.close(); 
        }
} m;

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
        //Vec vert[16];
        Flt radius;
        Flt alpha;
        int nsegments;
    public:
        Pointer() {
            pos[0] = 0.0;
            pos[1] = 0.2;
            pos[2] = 20.0;
            nsegments = 20;
            radius = 4;
        }
        void translate(float x, float y, float z) {
            pos[0] += x;
            pos[1] += y;
            pos[2] += z;
        }
}p;

class Smoke {
    public:
        Vec pos;
        Vec vert[16];
        Flt radius;
        int n;
        struct timespec tstart;
        Flt maxtime;
        Flt alpha;
        Flt dist;
        Smoke() { }
};

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
        int sorting;
        int billboarding;
        Camera camera;
        GLfloat lightPosition[4];
        BlenderObject objs[10];
        int xDirs[10];
        struct timespec smokeStart, smokeTime, healthStart, healthTime;
        Smoke *smoke;
        int nsmokes;
        Flt health;
        Flt cameramat[4][4];
        ~Global() {
            if (smoke)
                delete [] smoke;
        }
        Global() {
            //constructor
            moveInc = 0.0;
            xres=640;
            yres=480;
            aspectRatio = (GLfloat)xres / (GLfloat)yres;
            VecMake(0.0, 15.0, 75.0, cameraPos);
            cameraAngle = 0.0;
            sorting = 1;
            billboarding = 1;
            health = MAX_HEALTH;
            
            //VecMake(0.0, 1.0, 8.0, cameraPosition);
            //light is up high, right a little, toward a little
            VecMake(100.0f, 240.0f, 40.0f, lightPosition);
            lightPosition[3] = 1.0f;
            clock_gettime(CLOCK_REALTIME, &smokeStart);
            nsmokes = 0;
            smoke = new Smoke[MAX_SMOKES];
        }
        void init_opengl();
        void init();
        void check_mouse(XEvent *e);
        int check_keys(XEvent *e);
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
    //Place general program initializations here.
    srand(time(0));
    for (int i=0; i<10; i++) {
        int randNum = (rand() % (20+1+20)) + -20;
        g.xDirs[i] = randNum;
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
    static int shift = 0;
    static int tab = 0;
    if (e->type == KeyPress) {
        int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
        if (key == XK_Shift_L) {
            shift = 1;
            return 0;
        } 
        if (key == XK_Tab) {
            tab = 1;
            return 0;
        }
        if (key == XK_a) {
            g.health -= 0.02;
        }
    }
    if (e->type == KeyRelease) {
        int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
        if (key == XK_Shift_L) {
            shift = 0;
            return 0;
        }	
        if (key == XK_Tab) {
            tab = 0;
            return 0;
        }
        if (key == XK_a) {
            g.health -= 0.0002;
        }
    }
    if (e->type == KeyPress) {
        int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
        switch(key) {
            case XK_1:
                break;
            case XK_Right:
                // g.camera.lookLeftRight(0.05);
                // g.camera.moveLeftRight(1.0);
                p.translate(0.0,0.0,-1.0);
                // if (shift) {
                //      g.camera.lookLeftRight(-0.1);
                // } else {
                //      g.camera.moveLeftRight(1.0);
                // }
                break;
            case XK_Left:
                // g.camera.lookLeftRight(-0.05);
                // g.camera.moveLeftRight(-1.0);
                p.translate(0.0,0.0,1.0);
                // if (shift) {
                //      g.camera.lookLeftRight(0.1);
                // } else {
                //      g.camera.moveLeftRight(-1.0);
                // }
                break;
            case XK_Up:
                //g.camera.position[1] += 0.2;
                p.translate(-1.0,0.0,0.0);
                // if (shift) {
                //     g.camera.lookUpDown(-0.1);
                // } else if (tab) {
                //     g.camera.moveForwardBack(1.0);
                // } else {
                //     g.camera.translate(0.0, 0.2, 0.0);
                // }
                break;
            case XK_Down:
                p.translate(1.0,0.0,0.0);
                // if (shift) {
                //     g.camera.lookUpDown(0.1);
                // } else if (tab) {
                //     g.camera.moveForwardBack(-1.0);
                // } else {
                //     g.camera.translate(0.0, -0.2, 0.0);
                // }
                break;
            case XK_r:
                break;
            case XK_Escape:
                return 1;
        }
    }
    return 0;
}

void matrixFromAxisAngle(const Vec v, Flt ang, Matrix m)
{
    // arguments
    // v   = vector indicating the axis
    // ang = amount of rotation
    // m   = matrix to be updated
    // This source was used during research...
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
    //
    struct Axisangle {
        Flt angle;
        Flt x,y,z;
    } a1;
    a1.x = v[0];
    a1.y = v[1];
    a1.z = v[2];
    a1.angle = ang;
    //
    Flt c = cos(a1.angle);
    Flt s = sin(a1.angle);
    Flt t = 1.0 - c;
    m[0][0] = c + a1.x * a1.x * t;
    m[1][1] = c + a1.y * a1.y * t;
    m[2][2] = c + a1.z * a1.z * t;
    //
    Flt tmp1 = a1.x * a1.y * t;
    Flt tmp2 = a1.z * s;
    m[1][0] = tmp1 + tmp2;
    m[0][1] = tmp1 - tmp2;
    //
    tmp1 = a1.x * a1.z * t;
    tmp2 = a1.y * s;
    m[2][0] = tmp1 - tmp2;
    m[0][2] = tmp1 + tmp2;
    tmp1 = a1.y * a1.z * t;
    tmp2 = a1.x * s;
    m[2][1] = tmp1 + tmp2;
    m[1][2] = tmp1 - tmp2;
}

void identity33(Matrix m)
{
    m[0][0] = m[1][1] = m[2][2] = 1.0f;
    m[0][1] = m[0][2] = m[1][0] = m[1][2] = m[2][0] = m[2][1] = 0.0f;
}

void yy_transform(const Vec rotate, Matrix a)
{
    //This function applies a rotation to a matrix.
    //It actually concatenates a transformation to the matrix.
    //Call this function first, then call trans_vector() to apply the
    //rotations to an object or vertex.
    //
    if (rotate[0] != 0.0f) {
        Flt ct = cos(rotate[0]), st = sin(rotate[0]);
        Flt t10 = ct*a[1][0] - st*a[2][0];
        Flt t11 = ct*a[1][1] - st*a[2][1];
        Flt t12 = ct*a[1][2] - st*a[2][2];
        Flt t20 = st*a[1][0] + ct*a[2][0];
        Flt t21 = st*a[1][1] + ct*a[2][1];
        Flt t22 = st*a[1][2] + ct*a[2][2];
        a[1][0] = t10;
        a[1][1] = t11;
        a[1][2] = t12;
        a[2][0] = t20;
        a[2][1] = t21;
        a[2][2] = t22;
        return;
    }
    if (rotate[1] != 0.0f) {
        Flt ct = cos(rotate[1]), st = sin(rotate[1]);
        Flt t00 = ct*a[0][0] - st*a[2][0];
        Flt t01 = ct*a[0][1] - st*a[2][1];
        Flt t02 = ct*a[0][2] - st*a[2][2];
        Flt t20 = st*a[0][0] + ct*a[2][0];
        Flt t21 = st*a[0][1] + ct*a[2][1];
        Flt t22 = st*a[0][2] + ct*a[2][2];
        a[0][0] = t00;
        a[0][1] = t01;
        a[0][2] = t02;
        a[2][0] = t20;
        a[2][1] = t21;
        a[2][2] = t22;
        return;
    }
    if (rotate[2] != 0.0f) {
        Flt ct = cos(rotate[2]), st = sin(rotate[2]);
        Flt t00 = ct*a[0][0] - st*a[1][0];
        Flt t01 = ct*a[0][1] - st*a[1][1];
        Flt t02 = ct*a[0][2] - st*a[1][2];
        Flt t10 = st*a[0][0] + ct*a[1][0];
        Flt t11 = st*a[0][1] + ct*a[1][1];
        Flt t12 = st*a[0][2] + ct*a[1][2];
        a[0][0] = t00;
        a[0][1] = t01;
        a[0][2] = t02;
        a[1][0] = t10;
        a[1][1] = t11;
        a[1][2] = t12;
        return;
    }
}

void trans_vector(Matrix mat, const Vec in, Vec out)
{
    Flt f0 = mat[0][0] * in[0] + mat[1][0] * in[1] + mat[2][0] * in[2];
    Flt f1 = mat[0][1] * in[0] + mat[1][1] * in[1] + mat[2][1] * in[2];
    Flt f2 = mat[0][2] * in[0] + mat[1][2] * in[1] + mat[2][2] * in[2];
    out[0] = f0;
    out[1] = f1;
    out[2] = f2;
}

void cube(float w1, float h1, float d1)
{
    float w = w1 * 0.5;
    float d = d1 * 0.5;
    float h = h1 * 0.5;
    //notice the normals being set
    glBegin(GL_QUADS);
    // top
    glNormal3f( 0.0f, 1.0f, 0.0f);
    glVertex3f( w, h,-d);
    glVertex3f(-w, h,-d);
    glVertex3f(-w, h, d);
    glVertex3f( w, h, d);
    // bottom
    glNormal3f( 0.0f, -1.0f, 0.0f);
    glVertex3f( w,-h, d);
    glVertex3f(-w,-h, d);
    glVertex3f(-w,-h,-d);
    glVertex3f( w,-h,-d);
    // front
    glNormal3f( 0.0f, 0.0f, 1.0f);
    glVertex3f( w, h, d);
    glVertex3f(-w, h, d);
    glVertex3f(-w,-h, d);
    glVertex3f( w,-h, d);
    // back
    glNormal3f( 0.0f, 0.0f, -1.0f);
    glVertex3f( w,-h,-d);
    glVertex3f(-w,-h,-d);
    glVertex3f(-w, h,-d);
    glVertex3f( w, h,-d);
    // left side
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-w, h, d);
    glVertex3f(-w, h,-d);
    glVertex3f(-w,-h,-d);
    glVertex3f(-w,-h, d);
    // right side
    glNormal3f( 1.0f, 0.0f, 0.0f);
    glVertex3f( w, h,-d);
    glVertex3f( w, h, d);
    glVertex3f( w,-h, d);
    glVertex3f( w,-h,-d);
    glEnd();
    glEnd();
}


void drawGround()
{
    float w = 50.0;
    float d = 100.0;
    float w2 = w*.49;
    float d2 = d*.49;
    float x = -(1/2)*w;
    float y = 0.0;
    float z = (d/3)+25;
    // float xstart = x;
    // static int firsttime = 1;
    // if (firsttime) {
    //     firsttime = 0;
    //     printf("x: %f\n", x);
    // }
   
    glPushMatrix();
    glTranslatef(x, y, z);
    glColor3ub(34,139,34);
    glBegin(GL_QUADS);
    glNormal3f( 0.0f, 1.0f, 0.0f);
    glVertex3f( w2, 0.0, -d2);
    glVertex3f(-w2, 0.0, -d2);
    glVertex3f(-w2, 0.0,  d2);
    glVertex3f( w2, 0.0,  d2);
    glEnd();
    glPopMatrix();
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
    float theta = 2 * 3.1415926 / float(p.nsegments); 
	float c = cosf(theta); //precalculate the sine and cosine
	float s = sinf(theta);
	float t;

	float x = p.radius; //we start at angle = 0 
	float z = 0; 
    
	glBegin(GL_LINE_LOOP); 
	for(int ii = 0; ii < p.nsegments; ii++) 
	{ 
		glVertex3f(x + p.pos[0], p.pos[1], z + p.pos[2]);//output vertex 
        
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

    //create a key that decreases health more for a certain amount of time
    
//  glBegin(GL_QUADS);
//     glVertex3f(10, 35, 1); //x1, y1, z
//     glVertex3f(20, 35, 1); //x2, y1, z
//     glVertex3f(20, 30, 1); //x2, y2, z
//     glVertex3f(10, 30, 1); //x1, hy2, z
//     glEnd();
}
void drawSmoke()
{
    if (g.sorting) {
        for( int i= 0; i<g.nsmokes; i++){
            Flt formulaX = pow(g.smoke[i].pos[0] - g.cameraPos[0],2);
            Flt formulaY = pow(g.smoke[i].pos[1] - g.cameraPos[1],2);	
            Flt formulaZ = pow(g.smoke[i].pos[2] - g.cameraPos[2],2);

            g.smoke[i].dist = sqrt(formulaX + formulaY + formulaZ);
        }
        for(int i = 0; i<g.nsmokes -1; i++){
            for(int j=0; j<g.nsmokes-i-1; j++){
                if(g.smoke[j].dist < g.smoke[j+1].dist){
                    //swap(arr[j], arr[j+1]);
                    Smoke temp = g.smoke[j];
                    g.smoke[j] = g.smoke[j+1];
                    g.smoke[j+1] = temp;	
                }
            }
        }
    }
    //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i=0; i<g.nsmokes; i++) {
        glPushMatrix();
        glTranslatef(g.smoke[i].pos[0],g.smoke[i].pos[1],g.smoke[i].pos[2]);
        if (g.billboarding) {    
            // printf("inside of if\n");
            Vec v;
            VecSub(g.smoke[i].pos, g.cameraPos, v);

            Vec z = {0.0f, 0.0f, 0.0f};
            make_view_matrix(z, v, g.cameramat);

            float mat[16];
            mat[0] = g.cameramat[0][0];
            mat[1] = g.cameramat[0][1];
            mat[2] = g.cameramat[0][2];
            mat[4] = g.cameramat[1][0];
            mat[5] = g.cameramat[1][1];
            mat[6] = g.cameramat[1][2];
            mat[8] = g.cameramat[2][0];
            mat[9] = g.cameramat[2][1];
            mat[10] = g.cameramat[2][2];
            mat[3] = mat[7] = mat[11] = mat[12] = mat[13] = mat[14] = 0.0f;
            mat[15] = 1.0f;
            glMultMatrixf(mat);
        }
        glColor4ub(255, 0, 0, (unsigned char)g.smoke[i].alpha);
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0, 0.0, 1.0);
        for (int j=0; j<g.smoke[i].n; j++) {
            //each vertex of the smoke
            //glVertex3fv(g.smoke[i].vert[j]);
            vecNormalize(g.smoke[i].vert[j]);
            vecScale(g.smoke[i].vert[j], g.smoke[i].radius);
            glVertex3fv(g.smoke[i].vert[j]);
        }
        glEnd();
        glPopMatrix();
    }
    glDisable(GL_BLEND);
}

void make_a_smoke()
{
    if (g.nsmokes < MAX_SMOKES) {
        Smoke *s = &g.smoke[g.nsmokes];
        s->pos[0] = rnd() * 5.0 - 2.5;
        s->pos[2] = rnd() * 5.0 - 2.5;
        s->pos[1] = rnd() * 0.1 + 0.1;
        s->radius = rnd() * 1.0 + 0.5;
        s->n = rand() % 5 + 5;
        Flt angle = 0.0;
        Flt inc = (PI*2.0) / (Flt)s->n;
        for (int i=0; i<s->n; i++) {
            s->vert[i][0] = cos(angle) * s->radius;
            s->vert[i][1] = sin(angle) * s->radius;
            s->vert[i][2] = 0.0;
            angle += inc;
        }
        s->maxtime = 8.0;
        s->alpha = 100.0;
        clock_gettime(CLOCK_REALTIME, &s->tstart);
        ++g.nsmokes;
    }
}

void Global::physics()
{
    clock_gettime(CLOCK_REALTIME, &g.smokeTime);
    double d = timeDiff(&g.smokeStart, &g.smokeTime);
    if (d > 0.02) {
        //time to make another smoke particle
        make_a_smoke();
        timeCopy(&g.smokeStart, &g.smokeTime);
    }
    //move smoke particles
    for (int i=0; i<g.nsmokes; i++) {
        //smoke rising
        g.smoke[i].pos[1] += 0.015;
        g.smoke[i].pos[1] += ((g.smoke[i].pos[1]*0.24) * (rnd() * 0.075));
        //expand particle as it rises
        g.smoke[i].radius += g.smoke[i].pos[1]*0.002;
        //wind might blow particle
        if (g.smoke[i].pos[1] > 10.0) {
            g.smoke[i].pos[0] -= rnd() * 0.1;
        }
    }
    //check for smoke out of time
    int i=0;
    while (i < g.nsmokes) {
        struct timespec bt;
        clock_gettime(CLOCK_REALTIME, &bt);
        double d = timeDiff(&g.smoke[i].tstart, &bt);
        if (d > g.smoke[i].maxtime - 3.0) {
            g.smoke[i].alpha *= 0.95;
            if (g.smoke[i].alpha < 1.0)
                g.smoke[i].alpha = 1.0;
        }
        if (d > g.smoke[i].maxtime) {
            //delete this smoke
            --g.nsmokes;
            g.smoke[i] = g.smoke[g.nsmokes];
            continue;
        }
        ++i;
    }
    clock_gettime(CLOCK_REALTIME, &g.healthTime);
        d = timeDiff(&g.healthStart, &g.healthTime);
    if (d > 0.00001 && g.health > 0)  {
        //time to make another smoke particle
        g.health -= 0.0002;
        timeCopy(&g.healthStart, &g.healthTime);
    }
    //create a new timer that puts damage from a certain attack
}

void drawObject(BlenderObject obj,Flt x, Flt y, Flt z) {
    if(z < 50.0){
        glTranslatef(x,y,z);
    }
    obj.draw();
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
    //for documentation...
    Vec up = { 0.0, 1.0, 0.0 };
    gluLookAt(
            g.camera.position[0], g.camera.position[1], g.camera.position[2],
            g.camera.position[0]+g.camera.direction[0],
            g.camera.position[1]+g.camera.direction[1],
            g.camera.position[2]+g.camera.direction[2],
            up[0], up[1], up[2]);
    // gluLookAt(
    //         g.camera.position[0], g.camera.position[1], g.camera.position[2],
    //         -30.0, -30.0, 50.0,
    //         up[0], up[1], up[2]);
    glLightfv(GL_LIGHT0, GL_POSITION, g.lightPosition);
    //
    //drawGround();
    //
    drawPointer();
    //g.moveInc += 0.1;
    
    drawHealth();

    glColor3ub(100, 100, 100);
    drawObject(castle, 0.0,0.0,60.0);
    
    glColor3ub(20,255,20);  
    glScalef(4.0f,4.0f,5.0f);
    drawObject(terrain, 0.0,0.0,0.0);

    for(int i=0; i<10; i++) {
        drawObject(g.objs[i],g.xDirs[i],0.0,g.moveInc); 
    }
    glColor3ub(0, 0, 0);
    
    glDisable(GL_LIGHTING);
    // drawSmoke();
    glEnable(GL_LIGHTING);
    

    //switch to 2D mode
    //
    glViewport(0, 0, g.xres, g.yres);
    glMatrixMode(GL_MODELVIEW);   glLoadIdentity();
    glMatrixMode (GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0, g.xres, 0, g.yres);
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;
    glPopAttrib();
}



