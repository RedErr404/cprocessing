/*
 * graphics.cpp
 *
 *  Created on: Apr 28, 2011
 *      Author: esperanc
 */
#include "cprocessing.hpp"


/// This will link with the client's functions
extern void draw();
extern void setup();
extern void mousePressed();
extern void mouseReleased();
extern void mouseMoved();
extern void mouseDragged();
extern void keyPressed();
extern void keyReleased();

using namespace cprocessing;
namespace cprocessing
{

Style baseStyle;
bool mouseRecordFlag = true; // When to record or not the mouse position
//
// Global variables
//
ArrayList<std::string> args;
double mouseX = 0;  ///< Mouse x coordinate
double mouseY = 0;  ///< Mouse y coordinate
double pmouseX = 0;  ///< Previous mouse x coordinate
double pmouseY = 0;  ///< Previous mouse y coordinate
bool mousePressed = false; ///< Whether any mouse button is pressed
int mouseButton = LEFT; ///< Which button is pressed
bool keyPressed = false; ///< Whether a key was pressed
unsigned char key; ///< Which (ASCII) key was pressed
int keyCode;   ///< Code for the last pressed key

int width;     ///< window width
int height;    ///< window height
int screenWidth;
int screenHeight;

unsigned config = HALF_PIXEL_SHIFT | Y_DOWN; ///< Configuration flags

int framerate = 60; ///< Frames per second
double framedelay;  ///time in seconds between each frame
int frameCount = 0;
bool redrawflag = false;  ///draw next frame immediately
bool looping = true;   ///redisplay if true
int initialized = false; 	///< Whether or not initialization of glut took place

std::vector<Style> styles;
PixelColorBuffer pixels;
PImage screenBuffer;
GLFWwindow* win;

// Global OpenGL initialization code
static void gl_init ()
{
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Make it possible to overwrite pixels
    glDepthFunc(GL_LEQUAL);

    glShadeModel(GL_SMOOTH);

    glEnable(GL_MULTISAMPLE_ARB);

    // Helps when drawing in 3D with wireframe superimposed on the filled faces
    glPolygonOffset (1., -1.);

    // Cope with scaling transformations
    glEnable(GL_RESCALE_NORMAL);
    glEnable(GL_NORMALIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable the default additional ambient component
    float ambient [] = {0, 0, 0, 1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    // By default, y is flipped, so front is clockwise
    glFrontFace(GL_CW);

    // Make it possible to set material properties by using glcolor

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_SPECULAR);

    glClearColor (1.0, 1.0, 1.0, 0.0);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/// Called whenever window geometry is changed.
/// @param w New width of the window.
/// @param h New height of the window.
static void reshapeCallback (GLFWwindow* window, int w, int h)
{
    glViewport(0,0,w,h);

    width = w;
    height = h;

    gl_init();
    loadPixels();
}

/// Called whenever mouse moves
static void mousemotionCallback (GLFWwindow* window, double x, double y)
{
    if(mouseRecordFlag)
    {
        pmouseX = mouseX;
        pmouseY = mouseY;
        mouseX = ((x < 0) ? 0 : x); // red
        mouseY = ((y < 0) ? 0 : y);
        mouseRecordFlag = false;
    }
    ::mouseMoved();

    if(mousePressed) ::mouseDragged();
}

/// Called whenever mouse button is pressed
static void mouseCallback (GLFWwindow* window, int button, int action, int mods)
{
    mousePressed = action == GLFW_PRESS;

    if (button == GLFW_MOUSE_BUTTON_LEFT)        mouseButton = LEFT;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)  mouseButton = RIGHT;
    else                                         mouseButton = CENTER;

    if (mousePressed) ::mousePressed();
    else              ::mouseReleased();
}

/// Called whenever a key is pressed
static void keyboardCallback (GLFWwindow* window, int k, int scancode, int action, int mods)
{
    keyPressed = (action == GLFW_PRESS);
	
    switch (k)
    {
    case GLFW_KEY_F1:
        key = CODED;
        keyCode = F1;
        break;
    case GLFW_KEY_F2:
        key = CODED;
        keyCode = F2;
        break;
    case GLFW_KEY_F3:
        key = CODED;
        keyCode = F3;
        break;
    case GLFW_KEY_F4:
        key = CODED;
        keyCode = F4;
        break;
    case GLFW_KEY_F5:
        key = CODED;
        keyCode = F5;
        break;
    case GLFW_KEY_F6:
        key = CODED;
        keyCode = F6;
        break;
    case GLFW_KEY_F7:
        key = CODED;
        keyCode = F7;
        break;
    case GLFW_KEY_F8:
        key = CODED;
        keyCode = F8;
        break;
    case GLFW_KEY_F9:
        key = CODED;
        keyCode = F9;
        break;
    case GLFW_KEY_F10:
        key = CODED;
        keyCode = F10;
        break;
    case GLFW_KEY_F11:
        key = CODED;
        keyCode = F11;
        break;
    case GLFW_KEY_F12:
        key = CODED;
        keyCode = F12;
        break;
    case GLFW_KEY_LEFT:
        keyCode = LEFT;
        break;
    case GLFW_KEY_UP:
        keyCode = UP;
        break;
    case GLFW_KEY_RIGHT:
        keyCode = RIGHT;
        break;
    case GLFW_KEY_DOWN:
        keyCode = DOWN;
        break;
    case GLFW_KEY_PAGE_UP:
        keyCode = PAGEUP;
        break;
    case GLFW_KEY_PAGE_DOWN:
        keyCode = PAGEDOWN;
        break;
    case GLFW_KEY_HOME:
        keyCode = HOME;
        break;
    case GLFW_KEY_END:
        keyCode = END;
        break;
    case GLFW_KEY_INSERT:
        keyCode = INSERT;
        break;
    default:
        key = k;
    }

    if(action == GLFW_PRESS) ::keyPressed();
    else                    ::keyReleased();
    
}

static void errorCallback (int error, const char* description) {
    std::cout << error << " " << description << std::endl;	
}

//static void framebufferSizeCallback(GLFWwindow* window, int w, int h)
//{
//    glViewport(0,0,w,h);
//}

/// Sets up a window of the given size
/// @param w Desired window width in pixels.
/// @param h Desired window height in pixels.
/// @param name Desired window title.
void size (unsigned w, unsigned h, const char* name)
{
    width = w;
    height = h;

    if (initialized)
    {
        glfwSetWindowSize(win,width, height);
        glfwSetWindowTitle(win,name);
    }
    else
    {
	glfwSetErrorCallback(errorCallback);

        if(!glfwInit())
        {
            fprintf(stderr, "Failed to initialize GLFW\n");
            exit(EXIT_FAILURE);
        }

	//GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        //GLFWmonitor* monitor;
        //glfwGetVideoMode(&screenMode);
        //const GLFWvidmode* screenMode = glfwGetVideoMode(monitor);
        //screenWidth = screenMode.width;  //FIXME (returns 200??)
        //screenHeight = screenMode.height;  //FIXME (returns 201??)
        //screenWidth = 500;  //FIXME (returns 200??)
        //screenHeight = 500;  //FIXME (returns 201??)

        styles.push_back(baseStyle);
        bezierDetail(50);
        ellipseDetail(50);
        sphereDetail(30,30);
        stroke(0);
        fill(255);

        strokeWeight(1);
        colorMode(RGB);
        rectMode(CORNER);
        ellipseMode(CENTER);
        pixels.setBuffer(screenBuffer.texturebuffer);

        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	win = glfwCreateWindow(width, height, name, nullptr, nullptr);

        if(!win)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        /* Make the window's context current */
        glfwMakeContextCurrent(win);

        glfwSetWindowTitle(win, name);

        glfwSetWindowSizeCallback(win,reshapeCallback);
        glfwSetKeyCallback(win,keyboardCallback);
        glfwSetCursorPosCallback(win,mousemotionCallback);
        glfwSetMouseButtonCallback(win,mouseCallback);
	//glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);


        if(glewInit() != GLEW_OK)
        {
            fprintf(stderr, "Failed to initialize GLEW\n");
            exit(EXIT_FAILURE);
        }

        gl_init();
        initialized = true;
    }
}


/// Initializes and runs the application main event loop
void run()
{
    
    ::setup();

    if(initialized)
    {
        double frameTime = 0;
        double lastTime  = 0;
        while(initialized)
        {
            framedelay = (1.0f/(float)framerate);
            double elapsedTime = glfwGetTime();
            frameTime += (elapsedTime-lastTime);

            lastTime = elapsedTime;
            if((frameCount == 0 && looping) || (redrawflag) || (looping && frameTime > framedelay))
            {
                frameCount++;
                camera();
                perspective();
                noLights();
                ::draw();
                mouseRecordFlag = true;
                redrawflag = false;
                glfwSwapBuffers(win);
		glfwPollEvents();
                frameTime -= framedelay;
            }

            if(glfwGetKey(win,GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwDestroyWindow(win);
		glfwTerminate();
		exit(EXIT_SUCCESS);
	    }
        }
    }
    else
    {
        ::draw();
    }

    exit(EXIT_SUCCESS);
}
}
