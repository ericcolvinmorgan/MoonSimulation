#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <deque>
#include <string>
#include <strstream>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#include "glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"

#include "tifReader.h"
#include "glslprogram.h"

const char* WINDOWTITLE = { "Moon Simulation" };
const int INIT_WINDOW_SIZE = { 600 };
const GLfloat BACKCOLOR[] = { 0., 0., 0., 1. };

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };

// minimum allowable scale factor:

const float MINSCALE = { 0.05f };

// scroll wheel button values:

const int SCROLL_WHEEL_UP = { 3 };
const int SCROLL_WHEEL_DOWN = { 4 };

// equivalent mouse movement when we click a the scroll wheel:

const float SCROLL_WHEEL_CLICK_FACTOR = { 5. };

// active mouse buttons (or them together):

const int LEFT = { 4 };
const int MIDDLE = { 2 };
const int RIGHT = { 1 };

const float MOON_RADIUS_IN_KM = { 1737.4f };

struct Point {
	float X;
	float Y;
	float Z;
};

void InitGraphics();
void InitLists();

void Animate();
void Display();
void Keyboard(unsigned char, int, int);
void MouseButton(int, int, int, int);
void MouseMotion(int, int);
void Reset();
void Resize(int, int);
void Visibility(int);

void Sphere(float radius, int slices, int stacks);
unsigned char* BmpToTexture(char* filename, int* width, int* height);

Point CalculateCurrentView(std::deque<Point> points, float currentDistanceBetween, int offset = 0);
void DoRasterString(float x, float y, float z, char* s);

int		ActiveButton;			// current button that is down
int	MainWindow;
float	Scale;			// scaling factor
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees
float	HeightAdjust = 1.f;
GLuint SphereDataList;
GLuint MoonDataList;
GLSLProgram* MoonShaders;
unsigned char* MoonTexture;
//unsigned char* MoonDispTexture;
float* MoonDispTexture;
GLuint MoonTexName;
GLuint MoonDispTexName;

float DisplacementHeightDelta = 0.f;
float DisplacementWidthDelta = 0.f;

GLuint displacementBuf;
float* displacementArray = NULL;

std::deque<Point> TrackPoints;
float LastCheckpoint = 0.f;

#define MS_IN_THE_ANIMATION_CYCLE 10000

bool FlyByView = false;
bool ShowFlyByTrack = false;
bool UseDisplacementMapping = true;
bool UseBumpMapping = true;
bool UseLighting = true;

int main(int argc, char* argv[])
{
	// turn on the glut package: (do this before checking argc and argv since it might pull some command line arguments out)
	glutInit(&argc, argv);

	InitGraphics();
	InitLists();
	glutSetWindow(MainWindow);
	glutMainLoop();
	return 0;
}

void
InitGraphics()
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	// set the initial window configuration:

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(INIT_WINDOW_SIZE, INIT_WINDOW_SIZE);

	// open the window and set its title:

	MainWindow = glutCreateWindow(WINDOWTITLE);
	glutSetWindowTitle(WINDOWTITLE);

	// set the framebuffer clear values:

	glClearColor(BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3]);

	glutSetWindow(MainWindow);
	glutDisplayFunc(Display);
	glutReshapeFunc(Resize);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMotion);
	glutPassiveMotionFunc(NULL);
	glutVisibilityFunc(Visibility);
	glutEntryFunc(NULL);
	glutSpecialFunc(NULL);
	glutSpaceballMotionFunc(NULL);
	glutSpaceballRotateFunc(NULL);
	glutSpaceballButtonFunc(NULL);
	glutButtonBoxFunc(NULL);
	glutDialsFunc(NULL);
	glutTabletMotionFunc(NULL);
	glutTabletButtonFunc(NULL);
	glutMenuStateFunc(NULL);
	glutTimerFunc(-1, NULL, 0);
	glutIdleFunc(Animate);

	// init glew (a window must be open to do this):

#ifdef WIN32
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		fprintf(stderr, "glewInit Error\n");
	}
	else
		fprintf(stderr, "GLEW initialized OK\n");
	fprintf(stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

	//Load Moon Shader
	MoonShaders = new GLSLProgram();
	bool valid = MoonShaders->Create("moon.vert", "moon.frag");
	if (!valid)
	{
		fprintf(stderr, "Shader cannot be created!\n");
	}
	else
	{
		fprintf(stderr, "Shader created.\n");
	}
	MoonShaders->SetVerbose(false);
	GLint program = MoonShaders->GetProgram();

	//Load resources
	//auto tifrf = tifReader("./assets/ldem_4.tif");
	auto tifrf = tifReader("./assets/ldem_16.tif");
	auto intf = tifrf.get_values();
	auto valuef = tifrf.getValueCount();

	std::vector<float> f_values;
	for (unsigned int valNum = 0; valNum < valuef; valNum++)
	{
		f_values.push_back(*((float*)(intf + (valNum * sizeof(float)))));
	}
	
	displacementArray = new float[valuef];
	(void)memmove(&displacementArray[0], &f_values[0], valuef * sizeof(float));

	//Load Texture
	glGenTextures(1, &MoonTexName);
	//int nums = 1440, numt = 720;
	//MoonTexture = BmpToTexture("./assets/ldem_4_uint.bmp", &nums, &numt);
	//int nums = 4096, numt = 2048;
	//MoonTexture = BmpToTexture("./assets/lroc_color_poles_4k.bmp", &nums, &numt);
	int nums = 8192, numt = 4096;
	//MoonTexture = BmpToTexture("./assets/lroc_color_poles_8k.bmp", &nums, &numt);
	MoonTexture = BmpToTexture("./assets/lroc_color_poles_8k_flipped.bmp", &nums, &numt);
	//int nums = 16384, numt = 8192;
	//MoonTexture = BmpToTexture("./assets/lroc_color_poles_16k.bmp", &nums, &numt);
	//int nums = 27360, numt = 13680;
	//MoonTexture = BmpToTexture("./assets/lroc_color_poles.bmp", &nums, &numt);

	glBindTexture(GL_TEXTURE_2D, MoonTexName);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, nums, numt, 0, GL_RGB, GL_UNSIGNED_BYTE, MoonTexture);

	glGenTextures(1, &MoonDispTexName);
	//int nums_d = 1440, numt_d = 720;
	int nums_d = 5760, numt_d = 2880;
	//MoonDispTexture = BmpToTexture("./assets/ldem_4.bmp", &nums_d, &numt_d);
	//int nums_d = 6144, numt_d = 3072;
	MoonDispTexture = new float[nums_d * numt_d];

	(void)memmove(&MoonDispTexture[0], &displacementArray[0], nums_d * numt_d * sizeof(float));
	//Flip Rows - Not using flipped texture
	/*for(int i = 0; i < numt_d; i++)
	{
		int src_idx = i * nums_d;
		int dest_idx = ((numt_d - i) * nums_d) - nums_d;
		(void)memmove(&MoonDispTexture[dest_idx], &displacementArray[src_idx], nums_d * sizeof(float));
	}*/

	DisplacementWidthDelta = 1.f / nums_d;
	DisplacementHeightDelta = 1.f / numt_d;

	glBindTexture(GL_TEXTURE_2D, MoonDispTexName);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexImage2D(GL_TEXTURE_2D, 0, 3, nums_d, numt_d, 0, GL_RGB, GL_UNSIGNED_BYTE, MoonDispTexture);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, nums_d, numt_d, 0, GL_RED, GL_FLOAT, displacementArray);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, nums_d, numt_d, 0, GL_RED, GL_FLOAT, MoonDispTexture);

	//Draw Track Points
	int wayPoints = 16;
	//float radius = 1740.f;
	float radius = 1900.f;
	float radianStep = (2.f * M_PI) / wayPoints;

	//float orbits = 16.f;
	//float orbitRadianStep = (2.f * M_PI) / orbits;


	TrackPoints.clear();
	float currentRadian = 0.f;
	//Rotate Axis
	//for (int i = 0; i < wayPoints; i++)
	//{
	//	float currentX = std::cos(currentRadian);
	//	float currentY = std::sin(currentRadian);

	//	TrackPoints.push_back({currentX * radius, currentY * radius, 0});
	//	currentRadian += radianStep;
	//}

	//Figure 8
	for (int i = 0; i < wayPoints; i++)
	{
		float currentX = std::sin(currentRadian) * std::cos(currentRadian);
		float currentY = std::sin(currentRadian) * std::sin(currentRadian);
		float currentZ = std::cos(currentRadian);

		TrackPoints.push_back({currentX * radius, currentY * radius, currentZ * radius});
		currentRadian += radianStep;
	}
}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )

void
InitLists()
{
	SphereDataList = glGenLists(1);
	glNewList(SphereDataList, GL_COMPILE);
	Sphere(1.f, 360 * 4, 360 * 4);
	glEndList();

	MoonDataList = glGenLists(1);
	glNewList(MoonDataList, GL_COMPILE);
	Sphere(MOON_RADIUS_IN_KM, 360 * 4, 360 * 4);
	glEndList();
}

void
Animate()
{
	// put animation stuff in here -- change some global variables
	// for Display( ) to find:

	// force a call to Display( ) next time it is convenient:
	 
	int ms = glutGet(GLUT_ELAPSED_TIME); // milliseconds
	//ms %= MS_IN_THE_ANIMATION_CYCLE;
	//Time = (float)ms / (float)MS_IN_THE_ANIMATION_CYCLE; // [ 0., 1. )

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
Display()
{
	// erase the background:

	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
#ifdef DEMO_DEPTH_BUFFER
	if (DepthBufferOn == 0)
		glDisable(GL_DEPTH_TEST);
#endif

	// specify shading to be flat:
	glShadeModel(GL_FLAT);

	// set the viewport to a square centered in the window:
	GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
	GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = (vx - v) / 2;
	GLint yb = (vy - v) / 2;
	glViewport(xl, yb, v, v);

	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//if (WhichProjection == ORTHO)
	//	glOrtho(-3., 3., -3., 3., 0.1, 1000.);
	//else
	gluPerspective(90., 1., 0.1, MOON_RADIUS_IN_KM * 2.f);


	// place the objects into the scene:

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_FOG);

	// possibly draw the axes:


	// since we are using glScalef( ), be sure normals get unitized:
	glEnable(GL_NORMALIZE);

	// set the eye position, look-at position, and up-vector:
	float ms = glutGet(GLUT_ELAPSED_TIME);
	float currentPos = (ms - LastCheckpoint) / (float)MS_IN_THE_ANIMATION_CYCLE;

	if (currentPos >= 1.f)
	{
		TrackPoints.push_back(TrackPoints.front());
		TrackPoints.pop_front();
		LastCheckpoint += (float)MS_IN_THE_ANIMATION_CYCLE;
		currentPos -= 1.f;
	}

	auto currentEye = CalculateCurrentView(TrackPoints, currentPos);

	if (FlyByView)
	{
		auto currentLookAt = CalculateCurrentView(TrackPoints, currentPos + .01, 2);
		float orientation = 1.f;

		if (currentEye.Y < 0.f || currentLookAt.Y < 0.f)
			if (currentLookAt.X >= currentEye.X)
				orientation *= -1.f;

		gluLookAt(currentEye.X, currentEye.Y, currentEye.Z, currentLookAt.X, currentLookAt.Y, currentLookAt.Z, 0.f, orientation, 0.f);
	}
	else
	{

		gluLookAt(MOON_RADIUS_IN_KM * 1.5f, 0., 0., 0., 0., -5., 0., 1, 0.);
		glRotatef((GLfloat)Yrot, 0., 1., 0.);
		glRotatef((GLfloat)Xrot, 1., 0., 0.);

		if (Scale < MINSCALE)
			Scale = MINSCALE;
		glScalef((GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale);
	}

	MoonShaders->Use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, MoonTexName);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, MoonDispTexName);

	MoonShaders->SetUniformVariable("uShipPositionX", currentEye.X);
	MoonShaders->SetUniformVariable("uShipPositionY", currentEye.Y);
	MoonShaders->SetUniformVariable("uShipPositionZ", currentEye.Z);
	MoonShaders->SetUniformVariable("uHeightAdjust", HeightAdjust);
	MoonShaders->SetUniformVariable("uTexUnit", 0);
	MoonShaders->SetUniformVariable("uTexDispUnit", 1);
	MoonShaders->SetUniformVariable("uDisplacementMapping", UseDisplacementMapping);
	MoonShaders->SetUniformVariable("uBumpMapping", UseBumpMapping);
	MoonShaders->SetUniformVariable("uLighting", UseLighting);
	MoonShaders->SetUniformVariable("uWidthDelta", DisplacementWidthDelta);
	MoonShaders->SetUniformVariable("uHeightDelta", DisplacementHeightDelta);

	if (UseDisplacementMapping)
	{
		glCallList(SphereDataList);
	}
	else
	{
		glCallList(MoonDataList);
	}
	MoonShaders->Use(0);

	if (ShowFlyByTrack)
	{
		glColor3f(255.f, 0.f, 0.f);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < TrackPoints.size(); i++)
		{
			glVertex3f(TrackPoints.front().X, TrackPoints.front().Y, TrackPoints.front().Z);
			TrackPoints.push_back(TrackPoints.front());
			TrackPoints.pop_front();
		}
		glVertex3f(TrackPoints.front().X, TrackPoints.front().Y, TrackPoints.front().Z);
		glEnd();
	}

	// swap the double-buffered framebuffers:
	glutSwapBuffers();


	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !
	glFlush();
}

void
Keyboard(unsigned char c, int x, int y)
{
	switch (c)
	{

	case 'r':
	case 'R':
		Reset();
		break;

	case '-':
	case '_':
		HeightAdjust -= .1f;

		if(HeightAdjust < 0.f)
			HeightAdjust = 0.f;
		break;

	case '+':
	case '=':
		HeightAdjust += .1f;

		if (HeightAdjust > 10.f)
			HeightAdjust = 10.f;
		break;

	case 'v':
	case 'V':
		FlyByView = !FlyByView;
		break;

	case 'f':
	case 'F':
		ShowFlyByTrack = !ShowFlyByTrack;
		break;

	case 'd':
	case 'D':
		UseDisplacementMapping = !UseDisplacementMapping;
		break;

	case 'b':
	case 'B':
		UseBumpMapping = !UseBumpMapping;
		break;

	case 'l':
	case 'L':
		UseLighting = !UseLighting;
		break;

	default:
		fprintf(stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c);
	}

	// force a call to Display( ):
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
Resize(int width, int height)
{
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
Visibility(int state)
{
	if (state == GLUT_VISIBLE)
	{
		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}

void
MouseButton(int button, int state, int x, int y)
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT
	// get the proper button bit mask:

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		b = LEFT;		break;

	case GLUT_MIDDLE_BUTTON:
		b = MIDDLE;		break;

	case GLUT_RIGHT_BUTTON:
		b = RIGHT;		break;

	case SCROLL_WHEEL_UP:
		Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
		// keep object from turning inside-out or disappearing:
		if (Scale < MINSCALE)
			Scale = MINSCALE;
		break;

	case SCROLL_WHEEL_DOWN:
		Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
		// keep object from turning inside-out or disappearing:
		if (Scale < MINSCALE)
			Scale = MINSCALE;
		break;

	default:
		b = 0;
		fprintf(stderr, "Unknown mouse button: %d\n", button);
	}

	// button down sets the bit, up clears the bit:

	if (state == GLUT_DOWN)
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void MouseMotion(int x, int y)
{
	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if ((ActiveButton & LEFT) != 0)
	{
		Xrot += (ANGFACT * dy);
		Yrot += (ANGFACT * dx);
	}


	if ((ActiveButton & MIDDLE) != 0)
	{
		Scale += SCLFACT * (float)(dx - dy);

		// keep object from turning inside-out or disappearing:

		if (Scale < MINSCALE)
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow(MainWindow);
	glutPostRedisplay();

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void Reset()
{

}

Point CalculateCurrentView(std::deque<Point> points, float currentDistanceBetween, int offset)
{
	Point curvePoint;
	curvePoint.X = .5f * ((2.f * points[1+offset].X) + (currentDistanceBetween * (-points[0+offset].X + points[2+offset].X)) + (pow(currentDistanceBetween, 2) * ((2.f * points[0+offset].X) - (5.f * points[1+offset].X) + (4.f * points[2+offset].X) - points[3+offset].X)) + (pow(currentDistanceBetween, 3) * (-points[0+offset].X + (3.f * points[1+offset].X) - (3.f * points[2+offset].X) + points[3+offset].X)));
	curvePoint.Y = .5f * ((2.f * points[1+offset].Y) + (currentDistanceBetween * (-points[0+offset].Y + points[2+offset].Y)) + (pow(currentDistanceBetween, 2) * ((2.f * points[0+offset].Y) - (5.f * points[1+offset].Y) + (4.f * points[2+offset].Y) - points[3+offset].Y)) + (pow(currentDistanceBetween, 3) * (-points[0+offset].Y + (3.f * points[1+offset].Y) - (3.f * points[2+offset].Y) + points[3+offset].Y)));
	curvePoint.Z = .5f * ((2.f * points[1+offset].Z) + (currentDistanceBetween * (-points[0+offset].Z + points[2+offset].Z)) + (pow(currentDistanceBetween, 2) * ((2.f * points[0+offset].Z) - (5.f * points[1+offset].Z) + (4.f * points[2+offset].Z) - points[3+offset].Z)) + (pow(currentDistanceBetween, 3) * (-points[0+offset].Z + (3.f * points[1+offset].Z) - (3.f * points[2+offset].Z) + points[3+offset].Z)));
	return curvePoint;
}

void DoRasterString(float x, float y, float z, char* s)
{
	glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

	char c;			// one character to print
	for (; (c = *s) != '\0'; s++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
	}
}