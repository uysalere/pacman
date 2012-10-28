/************ HEADERS ***************/

/* Ordinary C stuff */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

/* GLUT headers */
#include </usr/include/GL/glut.h>

/* Maths library - remember to use -lm if building with GCC */
#include <math.h>

/* Time library - for random */
#include <time.h>

/* Our very own timer routines, used to measure frame rate, etc */
#include "Timer.h"

/************ GLOBALS AND DEFINES ***************/

/* the name of application */
static const char gApplicationName[] = "pacman";
static GLuint game_window;

/* the width and height of our window */
static float gWindowWidth = 800.0f;
static float gWindowHeight = 400.0f;

/* pi is a useful constant */
static const float Pi = 3.1415926535897932384626433832795f;
static const float PiTimesTwo = 6.283185307179586476925286766559f;
static const float PiOverTwo = 1.5707963267948966192313216916398f;

/* the id number of our openGL display list */
static GLuint gTerrainList = (GLuint)(-1);
static GLuint gPacman = (GLuint)(-1);
static GLuint gLQGhost = (GLuint)(-1);
static GLuint gMQGhost = (GLuint)(-1);
static GLuint gHQGhost = (GLuint)(-1);
static GLuint gHQDot = (GLuint)(-1);
static GLuint gHQFruit = (GLuint)(-1);

/* spline objects */
static GLUnurbsObj *theGhostNurb;
static GLUnurbsObj *theFruitNurb;
static const int hqGhostThreshold = 5;
static const int mqGhostThreshold = 15;

/* size of the one edge of the grid - in pixels */
static const int gridSize = 256;
static const int xCenter = gridSize / 2;
static const int yCenter = gridSize / 4;

/* projection constants */
static const int feet = 10;
static const int camDist = 10;
static int camHeight = camDist;
static int projection = 1;
static const int totalProjections = 3;
static int projectionAngle = 0;

/* game constants */
static int gameStart = 0;
static int gameWin = 0;
static int pacmanNewX = 0;
static int pacmanNewY = 0;
static char scoreBuf[10];
static char titleBuf[40] = "Pacman";
static char enterBuf[40] = "Press 's' to start";
static char cameraBuf[40] = "Press 'c' to change camera";
static char quitBuf[40] = "Press 'q' to exit";
static char loseBuf[40] = "GAME OVER";
static char winBuf[40] = "YOU WON!!";

/* our projection settings - we use these in our projection transformation */
static const float NearZPlane = 0.01;
static const float FarZPlane = float (gridSize*2);
static const float FieldOfViewInDegrees = 90.;

/* our animation and timing system */
static float gPacmanTimer = 0.;
static float gGhostTimer = 0.;
static const float slowGhostRate = 1.;
static const float initialGhostRate = 2.;
static float ghostRate = initialGhostRate;
static const int ghostRandomTime = 5;

/* heightMap */
static float heightMap[gridSize][gridSize];
static float colorVector[3];

/* height thresholds for snow and water */
static float snowThreshold;
static float waterThreshold;

/* scoring */
static int score = 0;
static const int dotScore = 10;
static const int ppillScore = 100;

/************ ADJACENCY LIST REP ***************/

typedef struct neighborList {
  struct node *left;		//in -x direction
  struct node *up;		//in z direction
  struct node *right;		//in x direction
  struct node *down;		//in -z direction
} Neighbors;

/* Represents a node. */
typedef struct node {
  int traversed;
  int x;
  int z;
  int ingame;              // as a boolean 0-none 1-exists
  int dot;                 // as a boolean 0-none 1-exists
  int ppill;               // as a boolean 0-noce 1-exists
  Neighbors nbor;
  int numadj;
  struct node *adj[4];
} Node;

/* distance between two parallel paths */
static const int DistPaths = 10;
static const int NodesPerLine = gridSize / DistPaths;

/* memory allocation for nodes */
static Node Nodes[NodesPerLine][NodesPerLine];
static int startX;
static int startZ;

/************ GAME OBJECTS *********************/

/* Represents a ghost. */
typedef struct ghost {
  int alive;
  float r;
  float g;
  float b;           
  int xMov;         
  int yMov;    
  int ghostTimer; 
  Node* cur;
} Ghost;

/* Represents pacman. */
typedef struct pacman {
  int alive;          
  int xMov;         
  int yMov;     
  Node* cur;
} Pacman;

/* memory allocation for objects. */
static Ghost Ghosts[4];
static Pacman Man;

/* variables needed for ghost creation. */
static int numDots = 0;
static int xPos;
static float yPos;
static int zPos;

/************ FUNCTION PROTOTYPES ***************/

/* GLUT callbacks.*/
void GameResize(int newWidth, int newHeight);
void GameKeyboardAction(unsigned char key, int mousex, int mousey);
void GameSpecialKeyAction(int key, int x, int y);
void GameDrawScene(void);

/* Our initialistation routines */
void InitialiseGLUT(int argc, char **argv);
void InitialiseOpenGL(void);
void InitialiseScene(void);

/* Our per-frame updating and rendering */
void UpdateFrame(void);

/* projection manipulations */
void projectionMenu(int value);
void setFirstPersonProjection(void);
void returnToMenu(int win);

/* Fractal geometry */
void SetHeightMap(void);
void DivideGrid(int x, int y, float size, float c1, float c2, float c3, float c4);
GLfloat* SetColor (float color_val, int x, int z);
void SetThresholds(void);

/* Drawing creation */
void renderBitmapString(float x, float y, void *font, char *string);
void DrawTerrain(void);
void DrawPacman(void);
void DrawFruit(void);
void DrawGhost(void);
void DrawDot(int quality);

/* adjacency list creation */
void createAdjacencyList(void);
Node* findPacmanStartNode (int starterX, int starterZ);
void traverseNeighbors(Node *node, int x, int y);
void placePowerpill(int x, int y);

/* ghost manipulations */
void createGhosts(void);
void randomize(Ghost* ghost);
Node* findStartNode (int starterX, int starterZ);
void checkStartDirection(Ghost* ghost);
void updateGhosts(void);
void checkGhostTimer(Ghost* ghost);

/* pacman manipulations */
void createPacman(void);
void refreshPacman(void);
void updatePacmanPosition(void);
void updatePacmanMovement(void);
void checkPacmanMovement(void);
void stopPacman(void);

/* scoring routines */
void addScores(void);
void checkCollision(void);

/************ INITIALISATION ROUTINES ***************/

/* Start GLUT, open a window to draw into, etc */
void InitialiseGLUT(int argc, char **argv)
{
  /* initialize glut */
  glutInit(&argc, argv);

  /* specify the display for our OpenGL application */
  glutInitDisplayMode(GLUT_RGBA				/* Use the RGBA colour model */
		      | GLUT_DOUBLE			/* Use double buffering */
		      | GLUT_DEPTH);			/* Use a depth buffer */

  /* define the size of the window */
  glutInitWindowSize(gWindowWidth, gWindowHeight);

  /* the position where the window will appear */
  glutInitWindowPosition(100,100);

  /* possible subwindow code
     main_window = glutCreateWindow(gApplicationName);
     glutDisplayFunc(MainDrawScene);
     glutReshapeFunc(MainResize);

     game_window = glutCreateSubWindow(main_window, 0, 0, gWindowWidth, gWindowHeight);
  */
  game_window = glutCreateWindow(gApplicationName);
  glutDisplayFunc(GameDrawScene);
  glutReshapeFunc(GameResize);
  glutKeyboardFunc(GameKeyboardAction);
  glutSpecialFunc(GameSpecialKeyAction);
  glutCreateMenu(projectionMenu);
  glutAddMenuEntry("Top of Pacman", 0);
  glutAddMenuEntry("2d View - From Top", 1);
  glutAddMenuEntry("2d View - From Side", 2);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

}

/* Do any one-time OpenGL initialisation we might require */
void InitialiseOpenGL()
{
  /* define the background, or "clear" colour, for our window  */
  glutSetWindow (game_window);
  glClearColor(0.7f, 0.7f, 0.7f, 0.0f);

  /*
    here we could do other "one time" initialisation routines
    for instance, we could enable lighting, enable COLOR_MATERIAL,
    enable light0, etc.
  */

  // smmoth shading and lighting enabled
  glShadeModel (GL_SMOOTH);
  glEnable(GL_LIGHTING);

  // changes on the original light
  // 0.1 of ambient light to give light to everything
  // specular lighting
  // in the +x, +y, -z direction
  GLfloat light_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
  GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_position[] = { 1.0, 1.0, -1.0, 0.0 };
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHT0);

  // second light source as sun
  // fixed poisiton and ambient lighting stronger than the original source
  GLfloat light1_position[] = { 300.0, 300.0, 150.0, 0.0 };
  GLfloat light1_ambient[] = { 0.4, 0.4, 0.4, 1.0 };
  glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
  glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
  glEnable(GL_LIGHT1);

  // depth test enabled to check the polygons behind
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  // used in order to normalize the normals so the projection and translation doesn't affect
  glEnable(GL_NORMALIZE);
  // enable the original colors to reflect and diffuse light
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

}

/* Create all the objects, textures, geometry, etc, that we will use */
void InitialiseScene(void)
{
  /* Create new lists, and store its ID number */
  gTerrainList = glGenLists(1);
  gPacman = glGenLists(2);
  gLQGhost = glGenLists(3);
  gMQGhost = glGenLists(4);
  gHQGhost = glGenLists(5);
  gHQDot = glGenLists(6);
  gHQFruit = glGenLists(7);

  // create terrain
  glNewList(gTerrainList, GL_COMPILE);
  DrawTerrain();
  glEndList();

  // create pacman
  glNewList(gPacman, GL_COMPILE);
  DrawPacman();
  glEndList();

  // create ghosts from splines
  theGhostNurb = gluNewNurbsRenderer();
  gluNurbsProperty(theGhostNurb, GLU_SAMPLING_METHOD, GLU_DOMAIN_DISTANCE);
  gluNurbsProperty(theGhostNurb, GLU_DISPLAY_MODE, GLU_FILL);
  gluNurbsProperty(theGhostNurb, GLU_CULLING, GL_TRUE);

  // high quality ghost
  gluNurbsProperty(theGhostNurb, GLU_U_STEP, 50.);
  gluNurbsProperty(theGhostNurb, GLU_V_STEP, 50.);
  glNewList(gHQGhost, GL_COMPILE);
  DrawGhost();
  glEndList();
 
  // medium quality ghost
  gluNurbsProperty(theGhostNurb, GLU_U_STEP, 20.);
  gluNurbsProperty(theGhostNurb, GLU_V_STEP, 20.);
  glNewList(gMQGhost, GL_COMPILE);
  DrawGhost();
  glEndList(); 

  // low quality ghost
  gluNurbsProperty(theGhostNurb, GLU_U_STEP, 5.);
  gluNurbsProperty(theGhostNurb, GLU_V_STEP, 5.);
  glNewList(gLQGhost, GL_COMPILE);
  DrawGhost();
  glEndList(); 

  gluDeleteNurbsRenderer(theGhostNurb); 

  /* create the banana from nurb spline*/
  theFruitNurb = gluNewNurbsRenderer();
  gluNurbsProperty(theFruitNurb, GLU_SAMPLING_METHOD, GLU_DOMAIN_DISTANCE);
  gluNurbsProperty(theFruitNurb, GLU_DISPLAY_MODE, GLU_FILL);
  gluNurbsProperty(theFruitNurb, GLU_CULLING, GL_TRUE);
  gluNurbsProperty(theFruitNurb, GLU_U_STEP, 20.);
  gluNurbsProperty(theFruitNurb, GLU_V_STEP, 20.);
  glNewList(gHQFruit, GL_COMPILE);
  DrawFruit();
  glEndList(); 

  gluDeleteNurbsRenderer(theFruitNurb); 

  /* create a dot */
  glNewList(gHQDot, GL_COMPILE);
  DrawDot(10);
  glEndList();
}

/************ GLUT CALLBACKS ***************/

/* Called whenever the size of the window changes */
void GameResize(int newWidth, int newHeight)
{
  glutSetWindow(game_window);

  /* instruct openGL to use the entirety of our window */
  glViewport(0, 0, newWidth, newHeight);

  /* set up the OpenGL projection matrix, including updated aspect ratio */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(FieldOfViewInDegrees, (float)newWidth / (float) newHeight, NearZPlane, FarZPlane);
}

/* Called whenever a keyboard button is pressed */
void GameKeyboardAction(unsigned char key, int mousex, int mousey)
{
  unsigned char keytest = tolower(key);
  /* allow the user to quit with the keyboard */
  if (keytest == 'q') {
    glutDestroyWindow(game_window);
    exit(0);
  /* allow the user to change the camera direction */
  } else if (keytest == 'c') {
    projectionMenu((projection + 1) % totalProjections);
  } else if (keytest == 's') {
    score = 0;
    projection = 0;
    gameStart = 1;
    gameWin = 0;
  }
}

/* Called whenever a special keyboard button is pressed */
void GameSpecialKeyAction(int key, int x, int y) {

  if (key == GLUT_KEY_UP) {
    projectionAngle = projectionAngle % 360;
  }
  else if(key == GLUT_KEY_DOWN) { 
    projectionAngle = (projectionAngle + 180) % 360 ;
  }
  else if(key == GLUT_KEY_RIGHT) { 
    projectionAngle = (projectionAngle - 90) % 360 ;
  }
  else if(key == GLUT_KEY_LEFT) {
    projectionAngle = (projectionAngle + 90) % 360 ;
  }

  // using cos and sin to find out the x and y coordinates of current direction
  pacmanNewX = sin(projectionAngle*M_PI/180);
  pacmanNewY = cos(projectionAngle*M_PI/180);

  glutPostRedisplay();
}

/*
  Called whenever the window needs to be redrawn.
*/
void GameDrawScene(void)
{
  // clear the background
  glutSetWindow(game_window);
  glClearColor(0.8, 0.8, 0.8, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);

  // set up projections
  glLoadIdentity();
  // get pacmans coordinates
  xPos = Man.cur->x + Man.xMov * gPacmanTimer;
  zPos = Man.cur->z + Man.yMov * gPacmanTimer;
  yPos =  (float) heightMap[xPos][zPos] + feet;

  if (projection == 0)
    // set a projection from pacmans perspective
    setFirstPersonProjection();
  else if (projection == 1)
    // set up a projection from above
    gluLookAt (xCenter, yCenter*(3), xCenter, 
	       xCenter, 0., xCenter, 
	       0.0, 0.0, 1.0);
  else
    // set up a projection from side
    gluLookAt (xCenter, yCenter, gridSize + yCenter, 
	       xCenter, yCenter, -1.0, 
	       0.0, 1.0, 0.0);

  // draw the terrain
  glPushMatrix();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glCallList(gTerrainList);
  glPopMatrix();

  // pacman
  glPushMatrix();
  glTranslatef(xPos, yPos, zPos);
  glCallList(gPacman);
  glPopMatrix();

  // dots
  glColor3f (1., 1., 1.);
  for (int i = 0; i < NodesPerLine; i++) {
    for (int j = 0; j < NodesPerLine; j++) {
      if (Nodes[i][j].dot > 0) {
	glPushMatrix();
	glTranslatef(Nodes[i][j].x, 
		     (float) heightMap[Nodes[i][j].x][Nodes[i][j].z] 
		     + (feet/4), 
		     Nodes[i][j].z);
	glCallList(gHQDot);
	glPopMatrix();
      }
    }
  }
	
  // fruits
  glColor3f (0.5, 1., 0.);
  for (int i = 0; i < NodesPerLine; i += NodesPerLine - 1) {
    for (int j = 0; j < NodesPerLine; j += NodesPerLine - 1) {
      if (Nodes[i][j].ppill > 0) {
	glPushMatrix();
	glTranslatef(Nodes[i][j].x, 
		     (float) heightMap[Nodes[i][j].x][Nodes[i][j].z] 
		     + feet, 
		     Nodes[i][j].z);
	glCallList(gHQFruit);
	glPopMatrix();
      }
    }
  }

  // ghost
  for(int i = 0; i < 4; i++) {
    glPushMatrix();
    glColor3f (Ghosts[i].r, Ghosts[i].g, Ghosts[i].b);
    
    // get ghosts corrdinates
    xPos = Ghosts[i].cur->x + Ghosts[i].xMov * gGhostTimer;
    zPos = Ghosts[i].cur->z + Ghosts[i].yMov * gGhostTimer;
    
    glTranslatef(xPos, (float) heightMap[xPos][zPos] + feet, zPos);
    
    // calculate its distance from pacman
    static int xDist, zDist;
    xDist = abs(Man.cur->x - xPos);
    zDist = abs(Man.cur->z - zPos);

    if ((xDist < hqGhostThreshold) || (zDist < hqGhostThreshold))
      // if it is close enough, high quality drawing
      glCallList(gHQGhost);
    else if ((xDist < mqGhostThreshold) || (zDist < mqGhostThreshold))
      // if it is close, medium quality drawing
      glCallList(gMQGhost);
    else 
      // otherwise low quality drawing
      glCallList(gLQGhost);

    glPopMatrix();
  }

  //set orthographic projection for score and main menu
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, gWindowWidth, gWindowHeight, 0);
  glMatrixMode(GL_MODELVIEW); 
    
  glPushMatrix();
  glLoadIdentity();

  // print score
  sprintf(scoreBuf, "Score: %d", score);
  glColor3f(1.0f, 1.0f, 1.0f);
  renderBitmapString(10, 40, GLUT_BITMAP_HELVETICA_18,scoreBuf);

  // main menu if game hasnt started
  if (gameStart < 1) {
    // stop movement
    gPacmanTimer = 0.;
    gGhostTimer = 0.;

    glColor3f(1., 0., 0.);
    if (gameWin > 0) {
      // if game is won, print YOU WON
      renderBitmapString(10, 20, GLUT_BITMAP_HELVETICA_18, winBuf);
      glColor3f(1., 0., 0.);
      renderBitmapString(10, 60, GLUT_BITMAP_HELVETICA_12, quitBuf);
    }
    else if (gameWin < 0) {
      // if game is lost, print GAME OVER
      renderBitmapString(10, 20, GLUT_BITMAP_HELVETICA_18, loseBuf);
      glColor3f(1., 0., 0.);
      renderBitmapString(10, 60, GLUT_BITMAP_HELVETICA_12, quitBuf);
    }
    else {
      // if game hasnt started, welcome
      renderBitmapString(10, 20, GLUT_BITMAP_HELVETICA_18,titleBuf);
      glColor3f(1., 0., 0.);
      renderBitmapString(10, 60, GLUT_BITMAP_HELVETICA_12, enterBuf);
    }
  } else {
    // write PACMAN 3D if you are already in game
    glColor3f(0.0f, 0.0f, 0.7f);
    renderBitmapString(10,20,GLUT_BITMAP_HELVETICA_18,titleBuf); 
    glColor3f(1., 0., 0.);
    renderBitmapString(10, 60, GLUT_BITMAP_HELVETICA_12, cameraBuf);
  }

  glPopMatrix();

  //return to modelview
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
	
  /* "double buffering" */
  glutSwapBuffers();
}

/************ FRAME UPDATES ***************/

/* Our main update function - called every time we go through our main loop */
void UpdateFrame(void)
{
  /* our timing information */
  unsigned int fps;
  float dt;

  /* force another redraw, so we are always drawing as much as possible */
  glutPostRedisplay();
	
  /* adjust our timer, which we might use for transforming our objects */
  dt = GetPreviousFrameDeltaInSeconds();
  dt = dt * (float) DistPaths;
  gPacmanTimer += dt;
  gGhostTimer += ghostRate * dt;

  // update ghosts current node when timer goes DistPaths pixels
  if (gGhostTimer > (float) DistPaths) {
    gGhostTimer = 0.;
    updateGhosts();
  }

  // update pacmans current node when timer goes DistPaths pixels
  if (gPacmanTimer > (float) DistPaths) {
    gPacmanTimer = 0.;
    refreshPacman();
  }
  // printf("gan: %d\n", (int) gPacmanTimer);
	
  /* timing information */
  if (ProcessTimer(&fps))
    {
      /* update our frame rate display */
      printf("FPS: %d\n", fps);
    }
}

/************ PROJECTION MANIPULATIONS ***************/

/* right click function to process the menu input to projection constant */
void projectionMenu (int value) {
  projection = value;
  glutPostRedisplay();
}

/* sets camera above pacman towards pacmans direction */
void setFirstPersonProjection (void) {
  /*
  xPos = Man.cur->x + Man.xMov * gPacmanTimer;
  zPos = Man.cur->z + Man.yMov * gPacmanTimer;
  yPos =  (float) heightMap[xPos][zPos] + feet;
  */
  
  if (Man.xMov > 0) {
    // look from -x to +x
    // check uphill or downhill
    /*if (heightMap[Man.cur->x][Man.cur->z] < heightMap[Man.cur->nbor.right->x][Man.cur->nbor.right->z])
      gluLookAt (xPos - camDist, yPos + (camHeight/2), zPos, 
		 xPos, yPos, zPos, 
		 0.0, 1.0, 0.0);
    else */
      gluLookAt (xPos - camDist, yPos + camHeight, zPos, 
		    xPos, yPos, zPos, 
		    0.0, 1.0, 0.0);
  }
  else if (Man.xMov < 0) {
    // look from +x to -x
    gluLookAt (xPos + camDist, yPos + camHeight, zPos, 
		    xPos, yPos, zPos, 
		    0.0, 1.0, 0.0);
  }
  else if (Man.yMov < 0) {
    // look from -z to +z
    gluLookAt (xPos, yPos + camHeight, zPos + camDist, 
	       xPos, yPos, zPos, 
	       0.0, 1.0, 0.0);
  }
  else if (Man.yMov > 0) {
    // look from +z to -z
    gluLookAt (xPos, yPos + camHeight, zPos - camDist, 
	       xPos, yPos, zPos, 
	       0.0, 1.0, 0.0);
  }
  else {
    // in the beginning look 2D
    gluLookAt (xPos, yPos + (2 * camDist), zPos, 
	       xPos, yPos, zPos, 
	       0.0, 0.0, 1.0);
    projectionAngle = 0;
  }
}

/* return to main menu when game is won or over */
void returnToMenu(int win) {  
  gameStart= 0;
  projection = 1;
  gameWin = win;
}

/************ FRACTAL GEOMETRY ***************/

/* A function to fill the heightMap values using fractal geometry */
void SetHeightMap(void)
{
  float c1, c2, c3, c4;
  srand ( time(NULL) );
		
  /* Assign the height of four corners of the initial grid */	
  c1 = (float) rand()/RAND_MAX;
  c2 = (float) rand()/RAND_MAX;
  c3 = (float) rand()/RAND_MAX;
  c4 = (float) rand()/RAND_MAX;
  DivideGrid(0, 0, gridSize, c1, c2, c3, c4);

  SetThresholds();
}

/* A function to recursively randomize */
void DivideGrid(int x, int y, float size, float c1, float c2, float c3, float c4)
{
  float e1, e2, e3, e4, mid, avg, max;
  float newSize = size / 2;

  // when each grid piece is bigger than a pixel
  if (size > 1)
    {		
      //Randomly displace the midpoint!
      avg = (c1 + c2 + c3 + c4) / 4;
      max = newSize / (float)(gridSize) * 3;	
      // special case for the first average to have occluded regions
      if (size == gridSize)	
	mid = 1.0f;
      else
	mid = avg + ((float) rand()/RAND_MAX - 0.5f) * max;

      //Make sure that the midpoint doesn't accidentally "randomly displaced" past the boundaries!
      if (mid < 0){
	mid = 0;
      }
      else if (mid > 1.0f){
	mid = 1.0f;
      }

      //Calculate the edges by averaging the two corners of each edge.
      e1 = (c1 + c2) / 2;
      e2 = (c2 + c3) / 2;
      e3 = (c3 + c4) / 2;
      e4 = (c4 + c1) / 2;
			
      //Do the operation over again for each of the four new grids.			
      DivideGrid(x, y, newSize, c1, e1, mid, e4);
      DivideGrid(x + newSize, y, newSize, e1, c2, e2, mid);
      DivideGrid(x + newSize, y + newSize, newSize, mid, e2, c3, e3);
      DivideGrid(x, y + newSize, newSize, e4, mid, e3, c4);
    }
  else
    {
      //The four corners of the grid piece will be averaged
      float c = (c1 + c2 + c3 + c4) / 4;
      heightMap[x][y] = c*((gridSize/2) -1);
    }
}

/* set the colors of terrain */
GLfloat* SetColor (float color_val, int x, int z) 
{
  static float r1;

  // snow
  if(color_val > snowThreshold ) {
    colorVector[0] = 0.9f;
    colorVector[1] = 0.9f;
    colorVector[2] = 0.9f;
  }
  // mountain
  else if(color_val > ((float) (gridSize/2) * 0.70f) && color_val > waterThreshold) {
    r1 = (float) rand()/RAND_MAX;
    r1 = 0.2f + (r1 * 0.2f);
    colorVector[0] = r1;
    colorVector[1] = r1 / 2.0f;
    colorVector[2] = 0.0f;
  }
  // grass
  else if(color_val > ((float) (gridSize/2) * 0.30f) && color_val > waterThreshold) {
    r1 = (float) rand()/RAND_MAX;
    r1 = 0.5f + (r1 * 0.2f);
    colorVector[0] = 0.0f;
    colorVector[1] = r1;
    colorVector[2] = 0.0f;
  }
  // soil
  else if(color_val > waterThreshold ){
    r1 = (float) rand()/RAND_MAX;
    r1 = 0.5f + (r1 * 0.2f);
    colorVector[0] = r1;
    colorVector[1] = r1;
    colorVector[2] = r1 / 5.0f;
  }
  // water surface levels the heightMap low values
  else {
    heightMap[x][z] = waterThreshold;
    colorVector[0] = 0.0f;
    colorVector[1] = 0.0f;
    colorVector[2] = 0.7f;
  }
  return colorVector;
}

/* Sets the height thresholds for snow and water areas */
void SetThresholds (void) {
  float values[(int)pow(ceil(gridSize / 10.0), 2)];
  int counter = 0;
  int i, j;
  for(i = 0; i < gridSize; i = i + 10)
    {
      for(j = 0; j < gridSize; j = j + 10)
	{
	  values[counter] = heightMap[i][j];
	  counter++;
	}
    }
  std::nth_element(values, values + (int)floor(sizeof(values) * 0.10/4), values + (int)sizeof(values)/4);
  waterThreshold = values[(int)floor(sizeof(values) * 0.10/4)];
  std::nth_element(values, values + (int)floor(sizeof(values) * 0.90/4), values + (int)sizeof(values)/4);
  snowThreshold = values[(int)floor(sizeof(values) * 0.90/4)]; 
}

/************ DRAWING CREATIONS ***************/

/* A function to render strings to fonts*/
void renderBitmapString(float x, float y, void *font, char *string) {
  char* c;
  glRasterPos2f(x, y);
  for (c=string; *c != '\0'; c++) {
    glutBitmapCharacter(font, *c);
  }
}

/* A function to draw the terrain using the heightMap values*/
void DrawTerrain(void)
{
  static int limit = gridSize -1;

  /* I can fill the terrain with triangles. */

  /* Triangles (x,z) (x+1,z) (x, z+1) Left */
  glBegin(GL_TRIANGLES);
  for(int x=0; x<limit; x++)
    for(int z=0; z<limit; z++)
      {
	glColor3fv(SetColor(heightMap[x][z], x, z));
	
	/* Calculating the normal vector
	   The 4 vectors surrounding the vertex is 
	   v1 (1, heightMap[x+1][z] - heightMap[x][z], 0)
	   v2 (0, heightMap[x][z+1] - heightMap[x][z], 1)
	   v3 (-1, heightMap[x-1][z] - heightMap[x][z], 0)
	   v4 (0, heightMap[x][z-1] - heightMap[x][z], -1)
	   v12 (heightMap[x+1][z] - heightMap[x][z], -1, 
	        heightMap[x][z+1] - heightMap[x][z])
	   v23 (- heightMap[x-1][z] - heightMap[x][z], 
	        -1, heightMap[x][z+1] - heightMap[x][z])
	   v34 (- heightMap[x-1][z] - heightMap[x][z], -1, 
	        - heightMap[x][z-1] - heightMap[x][z])
	   v41 (- heightMap[x+1][z] - heightMap[x][z], -1, 
	        - heightMap[x][z-1] - heightMap[x][z])
	   sum (-2 * heightMap[x-1][z] - heightMap[x][z], 
	        -4, 2 * heightMap[x][z+1] - heightMap[x][z-1])
	*/
	glNormal3f(-2 * (heightMap[x-1][z] - heightMap[x][z]), -4, 
		   2 * (heightMap[x][z+1] - heightMap[x][z-1]));
	glVertex3f(x, heightMap[x][z], z);
	glNormal3f(-2 * (heightMap[x][z] - heightMap[x+1][z]), -4, 
		   2 * (heightMap[x+1][z+1] - heightMap[x+1][z-1]));
	glVertex3f(x+1, heightMap[x+1][z], z);
	glNormal3f(-2 * (heightMap[x-1][z+1] - heightMap[x][z+1]), -4, 
		   2 * (heightMap[x][z+2] - heightMap[x][z]));
	glVertex3f(x, heightMap[x][z+1], z+1);
      }
  glEnd();

  /* Triangles (x,z) (x-1,z) (x, z-1) Right */
  glBegin(GL_TRIANGLES);
  for(int x=limit; x>0; x--)
    for(int z=limit; z>0; z--)
      {
	glColor3fv(SetColor(heightMap[x][z], x, z));

	/* Calculating the normal vector as above */
	glNormal3f(-2 * (heightMap[x-1][z] - heightMap[x][z]), -4, 
		   2 * (heightMap[x][z+1] - heightMap[x][z-1]));
	glVertex3f(x, heightMap[x][z], z);
	glNormal3f(-2 * (heightMap[x-2][z] - heightMap[x-1][z]), -4, 
		   2 * (heightMap[x-1][z+1] - heightMap[x-1][z-1]));
	glVertex3f(x-1, heightMap[x-1][z], z);
	glNormal3f(-2 * (heightMap[x-1][z-1] - heightMap[x][z-1]), -4, 
		   2 * (heightMap[x][z] - heightMap[x][z-2]));
	glVertex3f(x, heightMap[x][z-1], z-1);
      }
  glEnd();

  /* Then, I want to draw the sides. */
  /* Color */
  glColor3f (0.3, 0.3, 0.1);

  /* Side 1 x, 0 */
  glBegin(GL_POLYGON);
  glVertex3f(limit, 0, 0);
  for(int x=limit; x > -1; x--)
    glVertex3f(x, heightMap[x][0], 0);
  glVertex3f(0, 0, 0);
  glEnd(); 
  /* Side 2 0, z */
  glBegin(GL_POLYGON);
  glVertex3f(0, 0, 0);
  for(int z=0; z<gridSize; z++)
    glVertex3f(0, heightMap[0][z], z);
  glVertex3f(0, 0, limit);
  glEnd(); 
  /* Side 3 x, limit */
  glBegin(GL_POLYGON);
  glVertex3f(0, 0, limit);
  for(int x=0; x<gridSize; x++)
    glVertex3f(x, heightMap[x][limit], limit);
  glVertex3f(limit, 0, limit);
  glEnd();
  /* Side 4 limit, z */
  glBegin(GL_POLYGON);
  glVertex3f(limit, 0, 0);
  for(int z=0; z<gridSize; z++)
    glVertex3f(limit, heightMap[limit][z], z);
  glVertex3f(limit, 0, limit);
  glEnd();

}

/* drawing pacman with solid sphere */
void DrawPacman(void)
{
  glColor3f(1.0f, 1.0f, 0.0f);
  glutSolidSphere(5.0, 10, 10);
}

/* drawing fruit with NURBS spline */
void DrawFruit(void)
{
  // control points for bezier curve
  GLfloat fruit_ctlpoints[4][4][3] = { 
    { {-3., -2., 0.}, {-5., -1., -3.}, {-5., 2., -3.}, {-3., 5., -3.} },
    { {-1., -4., 0.}, {-2., -2., 3.},  {-2., 1., 3.}, {-1., 2., -3.} },
    { {1., -4., 0.}, {2., -3., 3.},  {2., -2., 3.}, {1., -1., -3.} },
    { {3., -2., 0.}, {5., 0., -3.}, {5., -1., -3.}, {3., 1., -3.} }
  };

  GLfloat knots[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
  gluBeginSurface(theFruitNurb);
  // map the vertices
  gluNurbsSurface(theFruitNurb, 
		  8, knots, 8, knots,
		  4 * 3, 3, &fruit_ctlpoints[0][0][0], 
		  4, 4, GL_MAP2_VERTEX_3);
  // map the normals
  gluNurbsSurface(theFruitNurb, 
		  8, knots, 8, knots,
		  4 * 3, 3, &fruit_ctlpoints[0][0][0], 
		  4, 4, GL_MAP2_NORMAL);
  gluEndSurface(theFruitNurb);
}

/* drawing ghost with NURBS spline */
void DrawGhost(void)
{

  // control points for bezier curve
  GLfloat ctlpoints[4][4][3] = { 
    { {-3., -5., 0.}, {-5., -2., -5.}, {-5., 2., -5.}, {-3., 3., 0.} },
    { {-2., -3., 0.}, {-2., -2., 5.},  {-2., 2., 5.}, {-2., 5., 0.} },
    { {2., -3., 0.}, {2., -2., -5.},  {2., 2., -5.}, {2., 5., 0.} },
    { {3., -5., 0.}, {5., -2., 5.}, {5., 2., 5.}, {3., 3., 0.} }
  };

  GLfloat knots[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
  gluBeginSurface(theGhostNurb);
  gluNurbsSurface(theGhostNurb, 
		  8, knots, 8, knots,
		  4 * 3, 3, &ctlpoints[0][0][0], 
		  4, 4, GL_MAP2_VERTEX_3);
  // map the vertices
  gluNurbsSurface(theGhostNurb, 
		  8, knots, 8, knots,
		  4 * 3, 3, &ctlpoints[0][0][0], 
		  4, 4, GL_MAP2_NORMAL);
  // map the normals
  gluEndSurface(theGhostNurb);

  // draw eyes with two white spheres
  glPushMatrix();
  glColor3f(1.0f, 1.0f, 1.0f);
  glTranslatef(1.5, 1.5, 0.);
  glutSolidSphere(0.5, 5, 5);
  glTranslatef(-3., 0., 0.);
  glutSolidSphere(0.5, 5, 5);
  glPopMatrix();

  /* for testing the points of bezier curve
     glPointSize(5.0);
     glColor3f(1.0, 0.0, 0.0);
     glBegin(GL_POINTS);
     for (i = 0; i < 4; i++) {
     for (j = 0; j < 4; j++) {
     glVertex3f(ctlpoints[i][j][0], 
     ctlpoints[i][j][1], ctlpoints[i][j][2]);
     printf("point: %lf %lf %lf", ctlpoints[i][j][0], 
     ctlpoints[i][j][1], ctlpoints[i][j][2]);
     }
     }
     glEnd();
  */
}

/* drawing dot with solid sphere */
void DrawDot(int quality)
{
  glColor3f(1.0f, 1.0f, 1.0f);
  glutSolidSphere(1.0, quality, quality);
}

/************ ADJACENCY LIST CREATION ***************/

/* creates the adjacency list */
void createAdjacencyList(void) {
  numDots = 0;
  static int gap = (gridSize - (DistPaths * NodesPerLine)) / 2;
  static int height;

  // create all the nodes with pos values and neighbors
  for(int i = 0; i < NodesPerLine; i++) {
    for(int j = 0; j < NodesPerLine; j++) {
      Nodes[i][j].x = gap + (i + (1/2.f)) * DistPaths;
      Nodes[i][j].z = gap + (j + (1/2.f)) * DistPaths;

      height = heightMap[Nodes[i][j].x][Nodes[i][j].z];
      if((height >= snowThreshold) || (height <= waterThreshold)) {
	Nodes[i][j].ingame = 0;
      } else
	Nodes[i][j].ingame = 1;

      Nodes[i][j].traversed = 0;
      Nodes[i][j].dot = 0;
      Nodes[i][j].ppill = 0;
      Nodes[i][j].numadj = 0;
    }
  }

  // Pacman's starting node
  startX = NodesPerLine / 2;
  startZ = NodesPerLine / 4;

  Man.cur = findPacmanStartNode(startX, startZ); 
  // find the nodes connected to the startNode
  traverseNeighbors(Man.cur, startX, startZ);

  // place powerpills on the corners 
  for (int i = 0; i < NodesPerLine; i += NodesPerLine - 1)
    for (int j = 0; j < NodesPerLine; j += NodesPerLine - 1)
      placePowerpill(i, j);
}

/* traverse through to all connected points */
void traverseNeighbors(Node *node, int x, int y) {
  // if node is already checked, there is no need to check again
  if (node->traversed > 0)
    return;

  node->traversed = 1;

  // if it is on traversal and ingame, there is a dot and neighbors
  // if it is out of the game, do not traverse any further
  if (node->ingame > 0) {
    node->dot = 1;
    numDots++;
    node->numadj = 0;
  }
  else return;
  
  // if we are not on the edge, check the left neigbor and recurse
  if (x > 0)
    if (Nodes[x-1][y].ingame > 0) {
      (node->nbor).left = &Nodes[x-1][y];
      node->adj[node->numadj] = &Nodes[x-1][y];
      node->numadj++;
      traverseNeighbors(&Nodes[x-1][y], x-1, y);
    }
    else (node->nbor).left = NULL;
  else (node->nbor).left = NULL;
  
  // if we are not on the edge, check the right neigbor and recurse
  if (x < NodesPerLine - 1)
    if (Nodes[x+1][y].ingame > 0) {
      (node->nbor).right = &Nodes[x+1][y];
      node->adj[node->numadj] = &Nodes[x+1][y];
      node->numadj++;
      traverseNeighbors(&Nodes[x+1][y], x+1, y);
    }
    else (node->nbor).right = NULL;
  else (node->nbor).right = NULL;
  
  // if we are not on the edge, check the up neigbor and recurse
  if (y < NodesPerLine - 1)
    if (Nodes[x][y+1].ingame > 0) {
      (node->nbor).up = &Nodes[x][y+1];
      node->adj[node->numadj] = &Nodes[x][y+1];
      node->numadj++;
      traverseNeighbors(&Nodes[x][y+1], x, y+1);
    }
    else (node->nbor).up = NULL;
  else (node->nbor).up = NULL;
  
  // if we are not on the edge, check the down neigbor and recurse
  if (y > 0)
    if (Nodes[x][y-1].ingame > 0) {
      (node->nbor).down = &Nodes[x][y-1];
      node->adj[node->numadj] = &Nodes[x][y-1];
      node->numadj++;
      traverseNeighbors(&Nodes[x][y-1], x, y-1);
    }
    else (node->nbor).down = NULL;
  else (node->nbor).down = NULL;
}

/* find a suitable start node for pacman */
Node* findPacmanStartNode (int starterX, int starterZ) {

  // while it is in game and has at least 1 neighbor

  while((Nodes[starterX][starterZ].ingame < 1) &&
	((Nodes[starterX+1][starterZ].ingame < 1) || 
	 (Nodes[starterX-1][starterZ].ingame < 1) || 
	 (Nodes[starterX][starterZ+1].ingame < 1) || 
	 (Nodes[starterX][starterZ-1].ingame < 1)) ) {
    // if startNode is out of game, check other possibilities on z line
    starterZ++;
  }
  return &Nodes[starterX][starterZ];
}

/* place poerpills on the corners */
void placePowerpill(int x, int y) {
  // check the corners
  // if there is a dot, replace it with a powerpill
  if (Nodes[x][y].dot > 0) {
    Nodes[x][y].dot = 0;
    Nodes[x][y].ppill = 1;
  }
}

/************ GHOST MANIPULATIONS ***************/

/* create ghosts by filling values */
void createGhosts(void) {
  startZ += NodesPerLine / 2;

  // create red ghost going down
  Ghosts[0].alive = 1;
  Ghosts[0].r = 1.;
  Ghosts[0].g = 0.;
  Ghosts[0].b = 0.; 
  Ghosts[0].xMov = 0; 
  Ghosts[0].yMov = -1; 
  Ghosts[0].ghostTimer = ghostRandomTime; 
  Ghosts[0].cur = findStartNode(startX, startZ);
  checkStartDirection(&Ghosts[0]);

  // create cyan ghost going up
  startZ++;
  Ghosts[1].alive = 1;
  Ghosts[1].r = 0.;
  Ghosts[1].g = 1.;
  Ghosts[1].b = 1.; 
  Ghosts[1].xMov = 0; 
  Ghosts[1].yMov = 1; 
  Ghosts[1].ghostTimer = ghostRandomTime; 
  Ghosts[1].cur = findStartNode(startX, startZ);
  checkStartDirection(&Ghosts[1]);

  // create orange ghost going right
  startX++;
  Ghosts[2].alive = 1;
  Ghosts[2].r = 1.;
  Ghosts[2].g = 0.5;
  Ghosts[2].b = 0.; 
  Ghosts[2].xMov = 1; 
  Ghosts[2].yMov = 0; 
  Ghosts[2].ghostTimer = ghostRandomTime; 
  Ghosts[2].cur = findStartNode(startX, startZ);
  checkStartDirection(&Ghosts[2]);

  // create pink ghost going left
  startX -= 2;
  Ghosts[3].alive = 1;
  Ghosts[3].r = 1.;
  Ghosts[3].g = 0.5;
  Ghosts[3].b = 0.5; 
  Ghosts[3].xMov = -1; 
  Ghosts[3].yMov = 0; 
  Ghosts[3].ghostTimer = ghostRandomTime; 
  Ghosts[3].cur = findStartNode(startX, startZ);
  checkStartDirection(&Ghosts[3]);
}

/* if a ghost hits a border, randomize its movement */
void randomize(Ghost* ghost) {
  static int rn, x, z, xdiff, zdiff;

  // stop the movement
  ghost->xMov = 0;
  ghost->yMov = 0;

  x = ghost->cur->x;
  z = ghost->cur->z;

  // pick a random node form its adjacent neighbors list
  rn = rand() % ghost->cur->numadj;

  // define the new direction towards the picked node
  xdiff = ghost->cur->adj[rn]->x - x;
  zdiff = ghost->cur->adj[rn]->z - z;

  if(xdiff > 0)
    ghost->xMov = 1;  
  else if (xdiff < 0)
    ghost->xMov = -1;
  else if (zdiff > 0)
    ghost->yMov = 1; 
  else if (zdiff < 0)
    ghost->yMov = -1;      
}

/* find a suitable starting node for a ghost */
Node* findStartNode (int starterX, int starterZ) {
  while((Nodes[starterX][starterZ].ingame < 1) && 
	(Nodes[starterX][starterZ].numadj < 1)) {
    // if startNode is out of game, check other possibilities on z line
    starterZ++;
  }
  return &Nodes[starterX][starterZ];
}

/* check the direction of the ghost in the start to see if its suitable*/
void checkStartDirection(Ghost* ghost) {

  // randomize if its not possible to move int hat direction
  if (ghost->xMov > 0) {
    if (ghost->cur->nbor.right == NULL)
      randomize(ghost);
  }
  else if (ghost->xMov < 0) {
    if (ghost->cur->nbor.left == NULL)
      randomize(ghost); 
  }
  else if (ghost->yMov > 0) {
    if (ghost->cur->nbor.up == NULL)
      randomize(ghost); 
  }
  else if (ghost->yMov < 0) {
    if (ghost->cur->nbor.down == NULL)
      randomize(ghost); 
  }
}

/* update ghosts positioning */
void updateGhosts(void) {
  for(int i = 0; i < 4; i++) {    
    if (Ghosts[i].xMov > 0) {
	Ghosts[i].cur = Ghosts[i].cur->nbor.right;
	checkGhostTimer(&Ghosts[i]);
      if (Ghosts[i].cur->nbor.right == NULL)
	randomize(&Ghosts[i]);
    }
    else if (Ghosts[i].xMov < 0) {
	Ghosts[i].cur = Ghosts[i].cur->nbor.left; 
	checkGhostTimer(&Ghosts[i]);
      if (Ghosts[i].cur->nbor.left == NULL)
	randomize(&Ghosts[i]);
    }
    else if (Ghosts[i].yMov > 0) {
	Ghosts[i].cur = Ghosts[i].cur->nbor.up; 
	checkGhostTimer(&Ghosts[i]);
      if (Ghosts[i].cur->nbor.up == NULL)
	randomize(&Ghosts[i]);
    }
    else if (Ghosts[i].yMov < 0) {
	Ghosts[i].cur = Ghosts[i].cur->nbor.down;  
	checkGhostTimer(&Ghosts[i]);
      if (Ghosts[i].cur->nbor.down == NULL)
	randomize(&Ghosts[i]); 
    }
  }
}

/* check if it time for the ghost to randomize */
void checkGhostTimer(Ghost* ghost) {
  ghost->ghostTimer--;
  if (ghost->ghostTimer == 0) {
    randomize(ghost);
    ghost->ghostTimer = ghostRandomTime;
  }
}


/************ PACMAN MANIPULATIONS ***************/

/* create pacman by filling its fields */
void createPacman(void) {
  Man.alive = 1; 
  Man.xMov = 0; 
  Man.yMov = 0; 
  // Man.cur defined earlier when creating adjacency matrix
}

/* refresh pacmans position */
void refreshPacman(void) {
  checkCollision();

  updatePacmanPosition();
  updatePacmanMovement();
  checkPacmanMovement();

  addScores();
  checkCollision();
}

/* update pacmans position */
void updatePacmanPosition(void) {
  // change the node according to the direction
  if (Man.xMov > 0) {
    Man.cur = Man.cur->nbor.right;
  }
  else if (Man.xMov < 0) {
    Man.cur = Man.cur->nbor.left;
  }
  else if (Man.yMov < 0) {
    Man.cur = Man.cur->nbor.down;
  }
  else if (Man.yMov > 0) {
    Man.cur = Man.cur->nbor.up;
  }
}

/* update pacmans direction */
void updatePacmanMovement(void) { 
  // get the read from keyboard and update pacmans movement in the next node
  if ((pacmanNewX != 0) || (pacmanNewY != 0)) {
    Man.xMov = pacmanNewX;
    Man.yMov = pacmanNewY;
    pacmanNewX = 0;
    pacmanNewY = 0;
  }
}

/* check pacmans direction to see if its suitable*/
void checkPacmanMovement(void) {
  // if it is trying to go to a border, stop

  if (Man.xMov > 0) {
    if (Man.cur->nbor.right == NULL)
      stopPacman();
    // if Pacman is going uphill slow down ghosts
    else if (heightMap[Man.cur->x][Man.cur->z] < 
	     heightMap[Man.cur->nbor.right->x][Man.cur->nbor.right->z])
      ghostRate=slowGhostRate;
    else
      ghostRate=initialGhostRate;
  }
  else if (Man.xMov < 0) {
    if (Man.cur->nbor.left == NULL)
      stopPacman();
    // if Pacman is going uphill slow down ghosts
    else if (heightMap[Man.cur->x][Man.cur->z] < 
	     heightMap[Man.cur->nbor.left->x][Man.cur->nbor.left->z])
      ghostRate=slowGhostRate;
    else
      ghostRate=initialGhostRate;
  }
  else if (Man.yMov < 0) {
    if (Man.cur->nbor.down == NULL)
      stopPacman();
    // if Pacman is going uphill slow down ghosts
    else if (heightMap[Man.cur->x][Man.cur->z] < heightMap[Man.cur->nbor.down->x][Man.cur->nbor.down->z])
      ghostRate=slowGhostRate;
    else
      ghostRate=initialGhostRate;
  }
  else if (Man.yMov > 0) {
    if (Man.cur->nbor.up == NULL)
      stopPacman();
    // if Pacman is going uphill slow down ghosts
    else if (heightMap[Man.cur->x][Man.cur->z] < heightMap[Man.cur->nbor.up->x][Man.cur->nbor.up->z])
      ghostRate=slowGhostRate;
    else
      ghostRate=initialGhostRate;
  }
}

/* stop pacmans movement */
void stopPacman(void) {
  Man.xMov = 0;
  Man.yMov = 0;
}

/************ SCORING ROUTINES ***************/

/* adding scores if pacman hits a dot or ppill */
void addScores(void) {
  if (Man.cur->dot > 0) {
    // add score
    score = score + dotScore;
    Man.cur->dot = 0;
    // decrease the number of dots
    numDots--;
  }

  if (Man.cur->ppill > 0) {
    // add score
    score = score + ppillScore;
    Man.cur->ppill = 0;
    numDots--;
  }

  // check if the game ended
  if (numDots < 1) {
    returnToMenu(1);
  }
}

void checkCollision(void) {
  for(int i = 0; i < 4; i++) {

    //collision when they are on the same node
    if (Man.cur == Ghosts[i].cur)
      returnToMenu(-1);

    //collision when pacman is moving to the ghosts node
    if (Man.xMov > 0) {
      if (Man.cur->nbor.right == Ghosts[i].cur)
      returnToMenu(-1);
    }
    else if (Man.xMov < 0) {
      if (Man.cur->nbor.left == Ghosts[i].cur)
      returnToMenu(-1);
    }
    else if (Man.yMov < 0) {
      if (Man.cur->nbor.down == Ghosts[i].cur)
      returnToMenu(-1);
    }
    else if (Man.yMov > 0) {
      if (Man.cur->nbor.up == Ghosts[i].cur)
      returnToMenu(-1);
    }
  }
}

/************ GOOD OLD INT MAIN() ***************/

int main(int argc, char **argv)
{
  /* Create the heightMap */
  SetHeightMap();
  
  /* Test printout for the fractal
    for(int x=0; x<gridSize; x++)
    for(int y=0; y<gridSize; y++)
    printf("heightMap[%d][%d] is %f\n", x, y, heightMap[x][y]);*/
  
  /* Create adjacency list */
  createAdjacencyList();
  printf("Number of dots is %d\n", numDots);

  /* create objects*/
  createPacman();
  createGhosts();

  /* Initialise GLUT - our window, our callbacks, etc */
  InitialiseGLUT(argc, argv);

  /* Do any one-time openGl initialisation that we might require */
  InitialiseOpenGL();

  /* Start up our timer. */
  InitialiseTimer();

  /* Start drawing the scene */
  InitialiseScene();

  /* Enter the main loop */
  glutIdleFunc(UpdateFrame);
  glutMainLoop();

  /* when the window closes, we will end up here. */
  return 0;
}
