//CMPSC 458 Heightmap Starter Code
#ifdef WIN32
// For VC++ you need to include this file as glut.h and gl.h refer to it
#include <windows.h>
// disable the warning for the use of strdup and friends
#pragma warning(disable:4996) 
#endif
#include <stdio.h>     // Standard Header For Most Programs
#include <stdlib.h>    // Additional standard Functions (exit() for example)
#include <iostream>
#include <GL/gl.h>     // The GL Header File
#include <GL/glu.h>    // The GL Utilities (GLU) header
#include <GL/glut.h>   // The GL Utility Toolkit (Glut) Header
// Interface to libpicio, provides functions to load/save jpeg files
#include <pic.h>
#include <sstream>
#include <iomanip>

using namespace std;

/* Enum for indices of skybox textures. */
enum Skybox {
	FRONT, BACK, LEFT, RIGHT, UP, DOWN
};

/* Simple struct to represent a 3D vector using GLfloat type. There is probably already one in GL library.*/
struct v3{
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

/* Texture ids for each face of the skybox*/
GLuint skyboxTextureId[6];

/* Whether or not we are recording. */
bool record = false;

/* Filename counter */
long fileCounter = 0;

void saveScreenshot(char *);

int g_iMenuId;

/* ----- USER INTERACTION VARS --------------------------------------------- */

/* State of the mouse */
int mousePos[2] = { 0, 0 };
int leftMouseButtonState = 0;    /* 1 if pressed, 0 if not */
int middleMouseButtonState = 0;
int rightMouseButtonState = 0;

/* State of color cycle*/
int colorCycle = 0;
int MAX_COLOR_CYCLE = 6;

/* Current control mode the program is in (what will happen when the mouse is
 * moved */
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE currentControlState = ROTATE;
v3 getColor(GLfloat h);

/* some global variable transformations to apply to the world */
/* we will change these values with keyboard and mouse interaction, and the display function will use them to transform the world */
GLfloat currentRotation[3] = { 0.0, 0.0, 0.0 };
GLfloat currentTranslation[3] = { 0.0, 0.0, -0.1 };
GLfloat currentScaling[3] = {0.01, 0.01, 0.01 };



/* ----- END USER INTERACTION VARS ----------------------------------------- */

/* ----- HEIGHTMAP VARS ---------------------------------------------------- */

/* Heightmap source image (pic.h uses the Pic* data structure) */
Pic * sourceImage;

/* Height of each pixel of source image */
GLfloat * heightArray;

/* Vertices for heightmap */
v3 * heightVertices;

/* Indices for tristrips of heightmap */
int * heightIndices;

/* ----- END HEIGHTMAP VARS ------------------------------------------------ */

/* ----- HELPER METHOD DECLARATIONS ---------------------------------------- */

// Draws the heightmap.
void drawHeightMap(void);

// Draws the skybox.
void drawSkyBox(void);

// Draws a face of the skybox.
void drawSkyFace(GLfloat texX[], GLfloat texY[], v3 skyBox[], int corner[], int special[]);

// Calculates and stores source image height data.
void calculateHeight(void);

// Generates the vertices for the heightmap tristrips
void generateHeightVertices(void);

// Generates the indices for the heightmap tristrips.
void generateHeightIndices(void);

/* ----- END DECLARATIONS -------------------------------------------------- */

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define Z_NEAR 0.1
#define Z_FAR 10000.0

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

// initGL will perform the one time initialization by
// setting some state variables that are not going to
// be changed
void InitGL(GLvoid)
{

	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);

	glClearColor(0.5f, 0.5f, 0.5f, 0.5f);				// Grey Background	

	//call our function to load a texture (you will have to do this for each texture you will load)
	loadTexture("texture/front.jpg", skyboxTextureId[FRONT]);
	loadTexture("texture/back.jpg", skyboxTextureId[BACK]);
	loadTexture("texture/left.jpg", skyboxTextureId[LEFT]);
	loadTexture("texture/right.jpg", skyboxTextureId[RIGHT]);
	loadTexture("texture/up.jpg", skyboxTextureId[UP]);
	loadTexture("texture/down.jpg", skyboxTextureId[DOWN]);
	/*if you plan to use Display lists this is a good place to
	  create them */
}


/* Main GL display loop.
 * This function is called by GL whenever it wants to update the display.
 * All of the drawing you do should be done within this function, or functions
 * that this calls
 */

void reshape(int w, int h)	//sets the reshape callback for the current window
{
	//TO DO: you can modify this function to do as you wish when window reshape occurs
	GLfloat aspect = (GLfloat)w / (GLfloat)h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat)w / (GLfloat)h, Z_NEAR, Z_FAR);
	glMatrixMode(GL_MODELVIEW);
}

void display(void)   // Create The Display Function. model, view and lighting goes here
{


	/* replace this code with your height field implementation */
	/* you may also want to precede it with your rotation/translation/scaling */

	/*example of world transformation*/
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Setting up coordinate system.
	// Z-axis points out from screen.
	// X-axis points to the left.
	// Y-axis points upwards.
	// In other words, the "negative Z" direction is pointing into the screen.
	gluLookAt(0,0,1,0,0,0,0,1,0);
	glRotatef(currentRotation[0], 1, 0, 0);
	glRotatef(currentRotation[1], 0, 1, 0);
	glRotatef(currentRotation[2], 0, 0, 1);
	glTranslatef(currentTranslation[0], currentTranslation[1], currentTranslation[2]);

	/* Clear buffers */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw heightmap
	int drawtriangle = 1;
	if (drawtriangle) {
		glPushMatrix();
		drawHeightMap();
		glPopMatrix();
	}

	int drawquad = 1;
	if (drawquad) {
		/* --- Drawing Skybox -------------------------- */
		glPushMatrix();
		drawSkyBox();
		glPopMatrix();
	}

	/* Swap buffers, so one we just drew is displayed */
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

void drawHeightMap(void){
	int width = sourceImage->nx;
	int height = sourceImage->ny;
	for (int x = 0; x < width - 1; x++){
		glBegin(GL_TRIANGLE_STRIP);
		for (int y = 0; y < height; ++y){
			GLfloat h = heightArray[x * width + y];
			glTexCoord2f(x * 1.0f, y * 1.0f);
			v3 color = getColor(h);
			glColor3f(color.x, color.y, color.z);
			//				glVertex3f(x * currentScaling[0] + currentTranslation[0],
			//					h * 10 * currentScaling[2] - currentTranslation[2],
			//					y * currentScaling[1] - currentTranslation[1]);
			glVertex3f(x * currentScaling[0],
				h * 10 * currentScaling[2],
				y * currentScaling[1]);

			h = heightArray[(x + 1) * width + y];
			glTexCoord2f((x + 1) * 1.0f, y * 1.0f);
			color = getColor(h);
			glColor3f(color.x, color.y, color.z);
			//				glVertex3f((x + 1) * currentScaling[0] + currentTranslation[0],
			//					h * 10 * currentScaling[2] - currentTranslation[2],
			//					y * currentScaling[1] - currentTranslation[1]);
			glVertex3f((x + 1) * currentScaling[0],
				h * 10 * currentScaling[2],
				y * currentScaling[1]);
		}
		glEnd();
	}
}

void drawSkyBox(void){
	GLfloat boxSize = 768.0;
	//GLfloat boxSize = 500;
	GLfloat halfSize = boxSize / 2;

	v3 skyBox[8];

	/* --- Calculating skybox vectors -------------- */

	// This can be initialized somewhere else, as we should only have to
	// calculate them once, even if we resize...?
	// Ain't nobody got time for that.

	/*

	0       1

	(front)

	3       2
	------------
	4       5

	(back)

	7       6

	*/

	skyBox[0].x = -halfSize;
	skyBox[0].y = halfSize;
	skyBox[0].z = -halfSize;

	skyBox[1].x = halfSize;
	skyBox[1].y = halfSize;
	skyBox[1].z = -halfSize;

	skyBox[2].x = halfSize;
	skyBox[2].y = -halfSize;
	skyBox[2].z = -halfSize;

	skyBox[3].x = -halfSize;
	skyBox[3].y = -halfSize;
	skyBox[3].z = -halfSize;

	skyBox[4].x = -halfSize;
	skyBox[4].y = halfSize;
	skyBox[4].z = halfSize;

	skyBox[5].x = halfSize;
	skyBox[5].y = halfSize;
	skyBox[5].z = halfSize;

	skyBox[6].x = halfSize;
	skyBox[6].y = -halfSize;
	skyBox[6].z = halfSize;

	skyBox[7].x = -halfSize;
	skyBox[7].y = -halfSize;
	skyBox[7].z = halfSize;

	/* --- Drawing skybox textures ----------------- */

	glEnable(GL_TEXTURE_2D); // Enable texturing from now on

	// Which corners of the skybox we are using
	int corner[4];

	// A potential workaround for the black skybox outline.
	int special[3];

	// Which vertices of the texture we are mapping
	GLfloat texX[4] = { 0.0, 0.0, 1.0, 1.0 };
	GLfloat texY[4] = { 0.0, 1.0, 1.0, 0.0 };

	// Front
	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[FRONT]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);

	corner[0] = 0;
	corner[1] = 3;
	corner[2] = 2;
	corner[3] = 1;
	special[0] = 1;
	special[1] = 1;
	special[2] = 0;
	drawSkyFace(texX, texY, skyBox, corner, special);
	glEnd();

	// Back
	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[BACK]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);

	corner[0] = 5;
	corner[1] = 6;
	corner[2] = 7;
	corner[3] = 4;
	special[0] = 1;
	special[1] = 1;
	special[2] = 0;
	drawSkyFace(texX, texY, skyBox, corner, special);
	glEnd();

	// Left
	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[RIGHT]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);

	corner[0] = 4;
	corner[1] = 7;
	corner[2] = 3;
	corner[3] = 0;
	special[0] = 0;
	special[1] = 1;
	special[2] = 1;
	drawSkyFace(texX, texY, skyBox, corner, special);
	glEnd();

	// Right
	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[LEFT]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);

	corner[0] = 1;
	corner[1] = 2;
	corner[2] = 6;
	corner[3] = 5;
	special[0] = 0;
	special[1] = 1;
	special[2] = 1;
	drawSkyFace(texX, texY, skyBox, corner, special);
	glEnd();

	// Up
	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[UP]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);

	corner[0] = 5;
	corner[1] = 4;
	corner[2] = 0;
	corner[3] = 1;
	special[0] = 1;
	special[1] = 0;
	special[2] = 1;
	drawSkyFace(texX, texY, skyBox, corner, special);
	glEnd();

	// Down
	glBindTexture(GL_TEXTURE_2D, skyboxTextureId[DOWN]);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);

	corner[0] = 7;
	corner[1] = 6;
	corner[2] = 2;
	corner[3] = 3;
	special[0] = 1;
	special[1] = 0;
	special[2] = 1;
	drawSkyFace(texX, texY, skyBox, corner, special);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void drawSkyFace(GLfloat texX[], GLfloat texY[], v3 skyBox[], int corner[], int special[]){
	// How much the faces overlap. Higher numbers will make the clipping noticable.
	GLfloat overlap = 1.1;
	for (int i = 0; i < 4; ++i){
		glTexCoord2f(texX[i], texY[i]);
		GLfloat vec[3];
		vec[0] = skyBox[corner[i]].x;
		vec[1] = skyBox[corner[i]].y;
		vec[2] = skyBox[corner[i]].z;
		for (int j = 0; j < 3; ++j){
			if (special[j]){
				if (vec[j] > 0){
					vec[j] += overlap;
				}
				else{
					vec[j] -= overlap;
				}
			}
		}
		glVertex3f(vec[0], vec[1], vec[2]);
	}
}

/* Fills out the heightArray with height values calculated from pixel data of source img. */
void calculateHeight(void){
	int width = sourceImage->nx;
	int height = sourceImage->ny;

	GLfloat rWeight = 0.33;
	GLfloat gWeight = 0.33;
	GLfloat bWeight = 0.34;

	heightArray = new GLfloat[width * height];

	for (int i = 0; i < width; ++i){
		for (int j = 0; j < height; ++j){
			GLfloat r = PIC_PIXEL(sourceImage, i, j, 0);
			GLfloat g = PIC_PIXEL(sourceImage, i, j, 1);
			GLfloat b = PIC_PIXEL(sourceImage, i, j, 2);
			GLfloat h = rWeight * r + gWeight * g + bWeight * b;
			// Heights are 0 <= height <= 1
			heightArray[width * j + i] = h / 256;
		//	cout << heightArray[width * j + i] << endl;
		}
	}
}

/* Returns a color depending on the color scheme selected. */
v3 getColor(GLfloat h){
	v3 toReturn;
	toReturn.x = h;
	toReturn.y = h;
	toReturn.z = h;
	switch (colorCycle){
	case 0: // White - Black
		// Nothing to change
		break;
	case 1: // Red - Black
		toReturn.y = 0;
		toReturn.z = 0;
		break;
	case 4: // White - Red
		toReturn.x = 1;
		break;
	case 2: // Green - Black
		toReturn.x = 0;
		toReturn.z = 0;
		break;
	case 5: // White - Green
		toReturn.y = 1;
		break;
	case 3: // Blue - Black
		toReturn.x = 0;
		toReturn.y = 0;
		break;
	case 6: // White - Blue
		toReturn.z = 1;
	}
	return toReturn;
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

/* converts mouse drags into information about rotation/translation/scaling
 * This is run by GL whenever the mouse is moved and a mouse button is being
 * held down.
 */
void mousedrag(int x, int y)
{
	int mousePosChange[2] = { x - mousePos[0], y - mousePos[1] };

	/* Check which state we are in. */
	switch (currentControlState)
	{
	case TRANSLATE:
		if (leftMouseButtonState)
		{
			currentTranslation[0] += mousePosChange[0] * 0.05;
			currentTranslation[1] -= mousePosChange[1] * 0.05;
		}
		if (middleMouseButtonState)
		{
			currentTranslation[2] += mousePosChange[1] * 0.05;
		}
		break;
	case ROTATE:
		if (leftMouseButtonState)
		{
			currentRotation[0] += mousePosChange[1];
			currentRotation[1] += mousePosChange[0];

			// Limiting rotation so that we don't get overflow/underflow errors.
			if (currentRotation[0] > 360){
				currentRotation[0] -= 360;
			}
			else if (currentRotation[0] < -360){
				currentRotation[0] += 360;
			}
			if (currentRotation[1] > 360){
				currentRotation[1] -= 360;
			}
			else if (currentRotation[1] < -360){
				currentRotation[1] += 360;
			}
		}
		if (middleMouseButtonState)
		{
			currentRotation[2] += mousePosChange[1];
		}
		break;
	case SCALE:
		if (leftMouseButtonState)
		{
			currentScaling[0] += mousePosChange[0] * 0.0005;
			currentScaling[1] += mousePosChange[1] * 0.0005;
		}
		if (middleMouseButtonState)
		{
			currentScaling[2] += mousePosChange[1] * 0.0005;
		}
		break;
	}

	/* Update stored mouse position */
	mousePos[0] = x;
	mousePos[1] = y;
}

/* This function is called by GL when the mouse moves passively.
 * "Passively" means that no mouse button is being held down */
void mouseidle(int x, int y)
{
	/* Update mouse position */
	mousePos[0] = x;
	mousePos[1] = y;
}



/* This is called by GL whenever a mouse button is pressed. */
void mousebutton(int button, int state, int x, int y)
{

	/* Check which button was pressed and update stored mouse state */
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButtonState = (state == GLUT_DOWN);
		break;
	case GLUT_MIDDLE_BUTTON:
		middleMouseButtonState = (state == GLUT_DOWN);
		break;
	case GLUT_RIGHT_BUTTON:
		rightMouseButtonState = (state == GLUT_DOWN);
		break;
	}

	/* Check what modifier keys (shift, ctrl, etc) are pressed and update the
	 * control mode based off of which keys are pressed */
	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		currentControlState = TRANSLATE;
		break;
	case GLUT_ACTIVE_SHIFT:
		currentControlState = SCALE;
		break;
	default:
		currentControlState = ROTATE;
		break;
	}

	/* Update stored mouse position */
	mousePos[0] = x;
	mousePos[1] = y;
}

/* This function will be called by GL whenever a keyboard key is pressed.
 * It recieves the ASCII value of the key that was pressed, along with the x
 * and y coordinates of the mouse at the time the key was pressed.
 */
void keyboard(unsigned char key, int x, int y) {

	/* User pressed quit key */
	if (key == 'q' || key == 'Q' || key == 27) {
		exit(0);
	}
	/* User pressed tab, so we cycle through colors. */
	if (key == 9){
		colorCycle++;
		if (colorCycle > MAX_COLOR_CYCLE){
			colorCycle = 0;
		}
	}
	/* If r is pressed, toggle recording. */
	if (key == 'r'){
		record = true;
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
	//you can put your  transformation code either here or in display() function
	//remember, there is only one model-view transformation matrix, and it applies to all objects you draw.
	//Therefore, you only need ONE copy of transformation code.   
	//For example, if you write
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//after your glulookat(),
	//the identity matrix will overwrite the previous transformation you did.

	/* make the screen update. */
	glutPostRedisplay();
}

// The ubiquituous main function.
int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{

	if (argc < 2)// if not specified, prompt for filename
	{
		char inputFile[999];
		printf("Input height file:");
		cin >> inputFile;
		sourceImage = jpeg_read(inputFile, NULL);
	}
	else //otherwise, use the name provided
	{
		/* Open jpeg (reads into memory) */
		sourceImage = jpeg_read(argv[1], NULL);
	}

	//TODO: intialize heightmap

	//this is an example of reading image data, you will use the pixel values to determine the height of the map at each node 
	//int heightmapxy=PIC_PIXEL(sourceImage , x, y, 0);
	//for color image data:
	//int red = PIC_PIXEL(sourceImage , 0, 0, 0);
	//int green = PIC_PIXEL(sourceImage , 0, 0, 1);
	//int blue = PIC_PIXEL(sourceImage , 0, 0, 2);
	//cout<<"\nThe (r,g,b) value at the upper left corner is ("<<red<<","<<green<<","<<blue<<")\n";
	calculateHeight();


	/*** The following are commands for setting up GL      ***/
	/*** No OpenGl call should be before this sequence!!!! ***/

	/* Initialize glut */
	glutInit(&argc, argv);

	/* Set up window modes */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	/* Set window position (where it will appear when it opens) */
	glutInitWindowPosition(0, 0);

	/* Set size of window */
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	/* Create the window */
	glutCreateWindow("CMPSC 458: Heightmap"); // Window Title (argv[0] for current directory as title)

	/**** Call to our initialization routine****/
	InitGL();

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/*sets the reshape callback for the current window*/
	glutReshapeFunc(reshape);

	/** allow the user to quit using the right mouse button menu **/
	/* Set menu function callback */
	g_iMenuId = glutCreateMenu(menufunc);

	/* Set identifier for menu */
	glutSetMenu(g_iMenuId);

	/* Add quit option to menu */
	glutAddMenuEntry("Quit", 0);

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
	glutKeyboardFunc(keyboard);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);

	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);

	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);



	glutMainLoop();          // Initialize The Main Loop

	return 0;
}

