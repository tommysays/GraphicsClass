#ifdef WIN32
// For VC++ you need to include this file as glut.h and gl.h refer to it
#include <windows.h>
#endif

#include <stdio.h>     // Standard Header For Most Programs
#include <stdlib.h>    // Additional standard Functions (exit() for example)
#ifdef __APPLE__

// This ONLY APPLIES to OSX
//
// Remember to add the -framework OpenGL - framework GLUT to
// to the gcc command line or include those frameworks
// using Xcode

#include <OpenGL/gl.h>     // The GL Header File
#include <OpenGL/glu.h>    // The GL Utilities (GLU) header
#include <GLUT/glut.h>   // The GL Utility Toolkit (Glut) Header
#else
#include <GL/gl.h>     // The GL Header File
#include <GL/glu.h>    // The GL Utilities (GLU) header
#include <GL/glut.h>   // The GL Utility Toolkit (Glut) Header
#endif
// Interface to libpicio, provides functions to load/save jpeg files
#include <pic.h>
#include "rc_spline.h"
#include <sstream>
#include <iomanip>

/* Here we will load the spline that represents the track */
rc_Spline g_Track;

/* glut Id for the context menu */
int g_iMenuId;


#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define M_PI           3.14159265358979323846
#define TIME_INTERVAL 10
#define SCALE 1
#define TRACK_WIDTH 0.03
#define CAMERA_HEIGHT 0.03
#define interpolation_resolution 1000.0

/* Enum for indices of skybox textures. */
enum Skybox {
	FRONT, BACK, LEFT, RIGHT, UP, DOWN
};

GLuint glow = 0;
GLuint NUM_COLORS = 5;
GLfloat ambientIntensity = 0.5;
GLfloat diffuseIntensity = 0.2;
GLfloat specIntensity = 0.4;

// Vars for determining speed.
GLfloat const EN = 100;
GLfloat MIN_HEIGHT = 1000.0;
GLfloat MAX_HEIGHT = -1000.0;
GLfloat HEIGHT_ENERGY_CONVERSION = 3.5;
GLfloat const ENERGY_SPEED_CONVERSION = 0.02;


GLuint skyboxTextureId[6];
GLuint skybox_display_index;
GLuint track_display_index;

bool record = false;
GLfloat currentRotation[3] = { 0.0, 0.0, 0.0 };
Vec3f cameraPos;

GLfloat TAU = 0.5;
GLuint counter = 0;
long fileCounter = 0;

struct CubicPolynomial{
	GLfloat x0, x1, x2, x3;
	GLfloat y0, y1, y2, y3;
	GLfloat z0, z1, z2, z3;
	CubicPolynomial(Vec3f p0, Vec3f p1, Vec3f p2, Vec3f p3){
		x0 = p1.x();
		x1 = (-1 * TAU) * p0.x() + TAU * p2.x();
		x2 = 2 * TAU * p0.x() + (TAU - 3) * p1.x() + (3 - 2 * TAU) * p2.x() + (-1 * TAU) * p3.x();
		x3 = (-1 * TAU) * p0.x() + (2 - TAU) * p1.x() + (TAU - 2) * p2.x() + TAU * p3.x();
		y0 = p1.y();
		y1 = (-1 * TAU) * p0.y() + TAU * p2.y();
		y2 = 2 * TAU * p0.y() + (TAU - 3) * p1.y() + (3 - 2 * TAU) * p2.y() + (-1 * TAU) * p3.y();
		y3 = (-1 * TAU) * p0.y() + (2 - TAU) * p1.y() + (TAU - 2) * p2.y() + TAU * p3.y();
		z0 = p1.z();
		z1 = (-1 * TAU) * p0.z() + TAU * p2.z();
		z2 = 2 * TAU * p0.z() + (TAU - 3) * p1.z() + (3 - 2 * TAU) * p2.z() + (-1 * TAU) * p3.z();
		z3 = (-1 * TAU) * p0.z() + (2 - TAU) * p1.z() + (TAU - 2) * p2.z() + TAU * p3.z();
	}
	Vec3f eval(GLfloat s)
	{
		GLfloat s2 = s*s;
		GLfloat s3 = s2 * s;
		return Vec3f(x0 + x1*s + x2*s2 + x3*s3,
			y0 + y1*s + y2*s2 + y3*s3,
			z0 + z1*s + z2*s2 + z3*s3);
	}
};

vector<Vec3f> interpolatedPoints;
GLfloat material_spec[] = { 1.0, 1.0, 1.0 };
GLfloat material_amb[] = { 1.0, 1.0, 1.0 };
GLfloat material_dif[] = { 1.0, 1.0, 1.0 };

/* OpenGL callback declarations 
definitions are at the end to avoid clutter */

void InitGL ( GLvoid );
void doIdle();
void display ( void );
void keyboardfunc (unsigned char key, int x, int y) ;
void menufunc(int value);
void reshape(int w, int h);
void makeSkyBox(GLuint index);
void loadTexture(char *filename, GLuint &textureID);
void interpolate(void);
void timer(int a);
void saveScreenshot(char *filename);


// The ubiquituous main function.
int main ( int argc, char** argv )   // Create Main Function For Bringing It All Together
{
	// get the the filename from the commandline.
	/*if (argc!=2)
	{
		printf("usage: %s trackfilname\n", argv[0]);
		system("PAUSE");
		exit(1);
	}*/

	/* load the track, this routine aborts if it fails */
	g_Track.loadSplineFrom("myTrack");

	/*** The following are commands for setting up GL      ***/
	/*** No OpenGl call should be before this sequence!!!! ***/

	/* Initialize glut */
	glutInit(&argc,argv);
	/* Set up window modes */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	/* Set window position (where it will appear when it opens) */
	glutInitWindowPosition(0,0);
	/* Set size of window */
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	/* Create the window */
	glutCreateWindow    ( "CMPSC 458: Rollercoaster" ); // Window Title (argv[0] for current directory as title)

	// Load skybox textures.
	loadTexture("texture/front.jpg", skyboxTextureId[FRONT]);
	loadTexture("texture/back.jpg", skyboxTextureId[BACK]);
	loadTexture("texture/left.jpg", skyboxTextureId[LEFT]);
	loadTexture("texture/right.jpg", skyboxTextureId[RIGHT]);
	loadTexture("texture/up.jpg", skyboxTextureId[UP]);
	loadTexture("texture/down.jpg", skyboxTextureId[DOWN]);

	// Load interpolated points.
	interpolate();

	/**** Call to our initialization routine****/
	InitGL ();
	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);
	/* Tell glut to use this reshape function */
	glutReshapeFunc(reshape);
	/* Tell glut to call timer function to update camera pos. */
	glutTimerFunc(50, timer, 100);
	/** allow the user to quit using the right mouse button menu **/
	/* Set menu function callback */
	g_iMenuId = glutCreateMenu(menufunc);
	/* Set identifier for menu */
	glutSetMenu(g_iMenuId);
	/* Add quit option to menu */
	glutAddMenuEntry("Quit",0);
	/* Add any other menu functions you may want here.  The format is:
	* glutAddMenuEntry(MenuEntry, EntryIndex)
	* The index is used in the menufunc to determine what option was selected
	*/
	/* Attach menu to right button clicks */
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	/* Set idle function.  You can change this to call code for your animation,
	* or place your animation code in doIdle */
	glutIdleFunc(doIdle);
	/* callback for keyboard input */
	glutKeyboardFunc(keyboardfunc);

	glutMainLoop        ( );          // Initialize The Main Loop

	return 0;
}

void timer(int a){
	Vec3f cur = interpolatedPoints[counter];
	counter++;
	GLfloat height = cur.y();
	GLfloat speed = ENERGY_SPEED_CONVERSION * (GLfloat)sqrt(EN - HEIGHT_ENERGY_CONVERSION * (height - MIN_HEIGHT));
	//GLfloat speed = pow((max_height - height) * HEIGHT_SPEED_CONVERSION, 2);
	if (counter >= interpolatedPoints.size()){
		counter = 0;
	}
	Vec3f next = interpolatedPoints[counter];
	cout << speed << endl;
	GLfloat distance = 0.0;
	//cout << "(" << cur.x() << ", " << cur.y() << ", " << cur.z() << ") : (" << next.x() << ", " << next.y() << ", " << next.z() << ")" << endl;
	while (distance < speed){
		
		Vec3f difference = next - cur;
		//cout << "dist : (" << difference.x() << ", " << difference.y() << ", " << difference.z() << ")" << endl;
		//GLfloat difDistance = sqrt(pow(difference.x(), 2) + pow(difference.y(), 2) + pow(difference.z(), 2));
		GLfloat difDistance = difference.Length();
		distance += difDistance;
		//cur = next;
		cur = interpolatedPoints[counter];
		counter++;
		if (counter >= interpolatedPoints.size()){
			counter = 0;
		}
		Vec3f next = interpolatedPoints[counter];
		//cout << "dif distance: " << difDistance << endl;
	}
	//cout << "out of while" << endl;
	if (counter >= interpolatedPoints.size()){
		counter = 0;
	}
	//cout << counter << endl;
	//cout << "camera pos: (" << cameraPos.x() << ", " << cameraPos.y() << ", " << cameraPos.z() << ")" << endl;
	cameraPos = Vec3f(interpolatedPoints[counter]);
	cameraPos.x() *= SCALE;
	cameraPos.y() *= SCALE;
	cameraPos.z() *= SCALE;
	//cout << cameraPos.x() << ", " << cameraPos.y() << ", " << cameraPos.z() << endl;

	//Vec3f previous = Vec3f(interpolatedPoints[(counter - 1) % interpolatedPoints.size()]);
	//Vec3f next = Vec3f(interpolatedPoints[(counter + 1) % interpolatedPoints.size()]);
	glutTimerFunc(TIME_INTERVAL, timer, 0);
}

void drawTrack(){
	glNewList(track_display_index, GL_COMPILE);
	glBegin(GL_TRIANGLE_STRIP);
	GLfloat mShininess[] = { 128 };
	glMaterialfv(GL_FRONT, GL_SHININESS, mShininess);
	glMaterialfv(GL_FRONT, GL_SPECULAR, material_spec);
	glColor3f(1.0, 0.0, 0.0);
	GLfloat counter = 0.0;
	GLfloat r = 1.0;
	GLfloat g = 0.0;
	GLfloat b = 0.0;
	GLfloat inc = 0.0015;
	bool redToGreen = true;
	bool greenToBlue = false;
	bool blueToRed = false;
	for (GLuint i = 0; i < interpolatedPoints.size(); ++i){
		if (redToGreen){
			r -= inc;
			g += inc;
		}
		else if (greenToBlue){
			g -= inc;
			b += inc;
		}
		else if (blueToRed){
			b -= inc;
			r += inc;
		}
		if (r >= 1.0){
			redToGreen = true;
			greenToBlue = false;
			blueToRed = false;
		}
		else if (g >= 1.0){
			redToGreen = false;
			greenToBlue = true;
			blueToRed = false;
		}
		else if (b >= 1.0){
			redToGreen = false;
			greenToBlue = false;
			blueToRed = true;
		}
		glColor3f(r, g, b);
		Vec3f diff = interpolatedPoints[(i + 1) % interpolatedPoints.size()] - interpolatedPoints[i];
		Vec3f up(0, 1, 0);
		Vec3f right;
		diff.Normalize();
		right.Cross3(right, diff, up);
		right.Normalize();
		up.Cross3(up, right, diff);
		up.Normalize();
		glNormal3f(up.x(), up.y(), up.z());
		right *= (float)TRACK_WIDTH;
		Vec3f cur = interpolatedPoints[i];
		Vec3f leftside = cur - right;
		right = cur + right;
		
		glTexCoord2f(i, 0);
		glVertex3f(leftside.x() * SCALE, leftside.y() * SCALE, leftside.z() * SCALE);
		glTexCoord2f(i, 1);
		glVertex3f(right.x() * SCALE, right.y() * SCALE, right.z() * SCALE);
	}
	glEnd();
	glEndList();
}

void interpolate(){
	pointVector points;
	GLfloat curx = 0, cury = 0, curz = 0;
	for (GLuint i = 1; i < g_Track.points().size(); ++i){
		curx += g_Track.points()[i].x();
		cury += g_Track.points()[i].y();
		curz += g_Track.points()[i].z();
		points.push_back(Vec3f(curx, cury, curz));
	}
	for (GLuint i = 0; i < points.size() ; ++i){
		int a = i;
		int b = (i + 1) % points.size();
		int c = (i + 2) % points.size();
		int d = (i + 3) % points.size();
		CubicPolynomial temp(points[i], points[b], points[c], points[d]);
		Vec3f first = points[b];
		Vec3f second = points[c];
		second -= first;
		GLfloat distance = second.Length();
		//GLfloat distance = sqrt(pow(points[i + 2].x() - points[i + 1].x(), 2) +
		//	pow(points[i + 2].y() - points[i + 1].y(), 2) +
		//	pow(points[i + 2].z() - points[i + 1].z(), 2));
		distance *= interpolation_resolution;
		for (GLfloat j = 0.0; j < distance; ++j){
			Vec3f vec = temp.eval(j / distance);
			interpolatedPoints.push_back(vec);
			if (vec.y() > MAX_HEIGHT){
				MAX_HEIGHT = vec.y();
			}
			if (vec.y() < MIN_HEIGHT){
				MIN_HEIGHT = vec.y();
			}
		}
	}
	HEIGHT_ENERGY_CONVERSION = EN / (MAX_HEIGHT - MIN_HEIGHT);
}

/* Here are all the standard OpenGL callbacks */

// initGL will perform the one time initialization by
// setting some state variables that are not going to
// be changed
void InitGL ( GLvoid )     // Create Some Everyday Functions
{

	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);

	GLuint index = glGenLists(2);
	track_display_index = index + 1;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_LIGHT0);
	GLfloat amb[] = { ambientIntensity , ambientIntensity, ambientIntensity };
	GLfloat ambPos[] = { 0.0, 1.0, 0.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_POSITION, amb);

	glEnable(GL_LIGHT1);
	GLfloat dif[] = { diffuseIntensity, diffuseIntensity, diffuseIntensity };
	GLfloat spec[] = { specIntensity, specIntensity, specIntensity };
	GLfloat pos[] = { 0.0, 1.5, -1.0, 0.0};
	glLightfv(GL_LIGHT1, GL_DIFFUSE, dif);
	glLightfv(GL_LIGHT1, GL_SPECULAR, spec);
	glLightfv(GL_LIGHT1, GL_POSITION, pos);
	//glDisable(GL_LIGHT1);


	// compile the display list, store a triangle in it
	//glNewList(index, GL_COMPILE);
	makeSkyBox(index);
	drawTrack();
}

void makeSkyBox(GLuint index){
	skybox_display_index = index;
	GLfloat size = 756;
	GLfloat halfSize = size / 2;

	pointVector skyBox;
	skyBox.push_back(Vec3f(-halfSize,  halfSize, -halfSize));
	skyBox.push_back(Vec3f( halfSize,  halfSize, -halfSize));
	skyBox.push_back(Vec3f( halfSize, -halfSize, -halfSize));
	skyBox.push_back(Vec3f(-halfSize, -halfSize, -halfSize));
	skyBox.push_back(Vec3f(-halfSize,  halfSize,  halfSize));
	skyBox.push_back(Vec3f( halfSize,  halfSize,  halfSize));
	skyBox.push_back(Vec3f( halfSize, -halfSize,  halfSize));
	skyBox.push_back(Vec3f(-halfSize, -halfSize,  halfSize));

	glNewList(skybox_display_index, GL_COMPILE);

	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[FRONT]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(skyBox[0].x(), skyBox[0].y(), skyBox[0].z());
	glTexCoord2f(0.0, 1.0);
	glVertex3f(skyBox[3].x(), skyBox[3].y(), skyBox[3].z());
	glTexCoord2f(1.0, 1.0);
	glVertex3f(skyBox[2].x(), skyBox[2].y(), skyBox[2].z());
	glTexCoord2f(1.0, 0.0);
	glVertex3f(skyBox[1].x(), skyBox[1].y(), skyBox[1].z());
	glEnd();

	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[BACK]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(skyBox[5].x(), skyBox[5].y(), skyBox[5].z());
	glTexCoord2f(0.0, 1.0);
	glVertex3f(skyBox[6].x(), skyBox[6].y(), skyBox[6].z());
	glTexCoord2f(1.0, 1.0);
	glVertex3f(skyBox[7].x(), skyBox[7].y(), skyBox[7].z());
	glTexCoord2f(1.0, 0.0);
	glVertex3f(skyBox[4].x(), skyBox[4].y(), skyBox[4].z());
	glEnd();

	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[RIGHT]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(skyBox[4].x(), skyBox[4].y(), skyBox[4].z());
	glTexCoord2f(0.0, 1.0);
	glVertex3f(skyBox[7].x(), skyBox[7].y(), skyBox[7].z());
	glTexCoord2f(1.0, 1.0);
	glVertex3f(skyBox[3].x(), skyBox[3].y(), skyBox[3].z());
	glTexCoord2f(1.0, 0.0);
	glVertex3f(skyBox[0].x(), skyBox[0].y(), skyBox[0].z());
	glEnd();

	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[LEFT]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(skyBox[1].x(), skyBox[1].y(), skyBox[1].z());
	glTexCoord2f(0.0, 1.0);
	glVertex3f(skyBox[2].x(), skyBox[2].y(), skyBox[2].z());
	glTexCoord2f(1.0, 1.0);
	glVertex3f(skyBox[6].x(), skyBox[6].y(), skyBox[6].z());
	glTexCoord2f(1.0, 0.0);
	glVertex3f(skyBox[5].x(), skyBox[5].y(), skyBox[5].z());
	glEnd();

	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[UP]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(skyBox[5].x(), skyBox[5].y(), skyBox[5].z());
	glTexCoord2f(0.0, 1.0);
	glVertex3f(skyBox[4].x(), skyBox[4].y(), skyBox[4].z());
	glTexCoord2f(1.0, 1.0);
	glVertex3f(skyBox[0].x(), skyBox[0].y(), skyBox[0].z());
	glTexCoord2f(1.0, 0.0);
	glVertex3f(skyBox[1].x(), skyBox[1].y(), skyBox[1].z());
	glEnd();

	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[DOWN]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(skyBox[7].x(), skyBox[7].y(), skyBox[7].z());
	glTexCoord2f(0.0, 1.0);
	glVertex3f(skyBox[6].x(), skyBox[6].y(), skyBox[6].z());
	glTexCoord2f(1.0, 1.0);
	glVertex3f(skyBox[2].x(), skyBox[2].y(), skyBox[2].z());
	glTexCoord2f(1.0, 0.0);
	glVertex3f(skyBox[3].x(), skyBox[3].y(), skyBox[3].z());
	glEnd();

	glEndList();
}

/*This function takes the name of a jpg file and a texture ID (by reference)*/
/*It creates a texture from the image specified and sets the ID specified to the value OpenGL generates for that texture*/
void loadTexture(char *filename, GLuint &textureID)
{
	Pic *pBitMap = pic_read(filename, NULL);

	if (pBitMap == NULL)
	{
		printf("Could not load the texture file\n");
		exit(1);
	}

	glEnable(GL_TEXTURE_2D); // Enable texturing
	glGenTextures(1, &textureID); // Obtain an id for the texture

	glBindTexture(GL_TEXTURE_2D, textureID); // Set as the current texture

	/* set some texture parameters */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	/* Load the texture into OpenGL as a mipmap. !!! This is a very important step !!! */
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, pBitMap->nx, pBitMap->ny, GL_RGB, GL_UNSIGNED_BYTE, pBitMap->pix);
	glDisable(GL_TEXTURE_2D);
	pic_free(pBitMap); // now that the texture has been copied by OpenGL we can free our copy
}

void reshape(int w, int h)	//sets the reshape callback for the current window
{
	//TO DO: you can modify this function to do as you wish when window reshape occurs
	GLfloat aspect = (GLfloat)w / (GLfloat)h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat)w / (GLfloat)h, .001, 50000);
	glMatrixMode(GL_MODELVIEW);
}


/* Main GL display loop.
* This function is called by GL whenever it wants to update the display.
* We will assume that the resulting image has already been created, and 
we will just paint it to the display.
*/

void display(void){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Change ambient lighting depending on user input
	GLfloat r = 0.0, g = 0.0, b = 0.0;
	if (glow == 4){
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHT0);
	}
	else{
		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHT0);
	}
	switch (glow){
	case 0:
		r = ambientIntensity;
		g = ambientIntensity;
		b = ambientIntensity;
		break;
	case 1:
		r = ambientIntensity;
		g = 0.0;
		b = 0.0;
		break;
	case 2:
		r = 0.0;
		g = ambientIntensity;
		b = 0.0;
		break;
	case 3:
		r = 0.0;
		g = 0.0;
		b = ambientIntensity;
		break;
	case 4:
		r = 0;
		g = 0;
		b = 0;
		break;
	}
	GLfloat amb[] = { r, g, b };
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Vec3f diff = interpolatedPoints[(counter + 1) % interpolatedPoints.size()];
	diff -= interpolatedPoints[counter];
	diff.Normalize();
	Vec3f up(0, 1, 0);
	Vec3f right;
	right.Cross3(right, diff, up);
	up.Cross3(up, right, diff);
	up.Normalize();
	Vec3f upHeight = up;
	upHeight *= (float)CAMERA_HEIGHT;
	gluLookAt(0, 0, 0, diff.x(), diff.y(), diff.z(), up.x(), up.y(), up.z());
	/*
	glRotatef(currentRotation[2], 1, 0, 0);
	glRotatef(currentRotation[1], 0, 1, 0);
	glRotatef(currentRotation[0], 0, 0, 1);
	*/
	upHeight *= (float)-1.0;
	upHeight = upHeight - cameraPos;
	glTranslatef(upHeight.x(), upHeight.y(), upHeight.z());
	//glTranslatef(-cameraPos.x(), -cameraPos.y(), -cameraPos.z());
	//cout << cameraPos.x() << ", " << cameraPos.y() << ", " << cameraPos.z() << endl;


	glEnable(GL_TEXTURE_2D);
	glPushMatrix();
	glCallList(skybox_display_index);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	glCallList(track_display_index);
	glPopMatrix();

	glutSwapBuffers();

	if (record){
		std::ostringstream makeString;
		makeString << std::setw(10) << std::setfill('0') << fileCounter++ << ".jpg";
		std::string fileName = makeString.str();
		char *name = new char[fileName.length() + 1];
		strcpy(name, fileName.c_str());
		saveScreenshot(name);
		delete[] name;
	}
}

/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
	int i;
	Pic *in = NULL;

	if (filename == NULL)
		return;

	/* Allocate a picture buffer */
	in = pic_alloc(WINDOW_WIDTH, WINDOW_HEIGHT, 3, NULL);

	printf("File to save to: %s\n", filename);

	/* Loop over each row of the image and copy into the image */
	for (i = WINDOW_HEIGHT - 1; i >= 0; i--) {
		glReadPixels(0, WINDOW_HEIGHT - 1 - i, WINDOW_WIDTH, 1, GL_RGB,
			GL_UNSIGNED_BYTE, &in->pix[i*in->nx*in->bpp]);
	}

	/* Output the file */
	if (jpeg_write(filename, in)) {
		printf("File saved Successfully\n");
	}
	else {
		printf("Error in Saving\n");
	}

	/* Free memory used by image */
	pic_free(in);
}

/* This function will be called by GL whenever a keyboard key is pressed.
* It recieves the ASCII value of the key that was pressed, along with the x
* and y coordinates of the mouse at the time the key was pressed.
*/
void keyboardfunc(unsigned char key, int x, int y) {

	/* User pressed quit key */
	if (key == 'q' || key == 'Q' || key == 27) {
		exit(0);
	}

	/* If r is pressed, toggle recording. */
	if (key == 'r'){
		record = !record;
	}

	// Tab was pressed; Cycle through glow colors.
	if (key == 9){
		glow++;
		if (glow >= NUM_COLORS){
			glow = 0;
		}
	}
}
/* Function that GL runs once a menu selection has been made.
* This receives the number of the menu item that was selected
*/
void menufunc(int value)
{
	switch (value)
	{
	case 0:
		exit(0);
		break;
	}
}

/* This function is called by GL whenever it is idle, usually after calling
* display.
*/
void doIdle()
{
	/* do some stuff... */

	/* make the screen update. */
	glutPostRedisplay();
}