// assign2.cpp : Defines the entry point for the console application.
//

/*
	CSCI 420 Computer Graphics - SPRING 2015
	Avinash Akka: USC ID: 3874-5774-01
	Assignment 2: Simulating a Roller Coaster
*/
/* Basic Idea behind my implementation is:
	* Load the track points using the text file
	* Create the spline points between P1 and P2 using 4 control points from the track file
	* Vary U between 0 to 1 with a constant increment of 0.001 so there are 1000 points between P1 and P2
	* Render the curve points as two tracks which in themselves are 3D elongated cuboid with four faces
	* Render the RailLine Planks i.e the horizontal planks which also are 3D Cudoids.
	* Create a SkyBox i.e the Scene inside a Cube of dimensions 100 X 100 X 100 
	* The walls of the Cube are Texture Mapped with JPEG images of 512X512 SIZE
	* SetUp basic Light setup
	* Align the camera such that it increments at a constant velocity maling it look like a ride
	* Use the goodRide spline for a good view.
	*/

#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>


double count = 0.0;

bool stop_counter = true; // Required for timeFunction to save screenshots. 
int Count;//Screenshots counter

bool FogUpStart = false; // Initially fog is not rendered unless requested

/* Similar to the 1st Assignment */
int g_iMenuId;
int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;  
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;

/*Similar to Assignment 1*/
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* represents one control point along the spline */
struct point { 
   double x;
   double y;
   double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

/* the spline array */
struct spline *g_Splines; 
/* total number of splines */
int g_iNumOfSplines; 

GLuint textures[] = {0, 1, 2, 3, 4, 5, 6};//Array to save texture addresses

const int Increments = 1000;//U varies between 0 & 1 over 1000 values

const double Track_Diff = 0.1;//Distance between the two rail tracks

const int Plank_Diff = Increments / 20;// Distance between two rail track planks
const double Plank_Thick = Track_Diff / 50;// thickness of the rail track plank
const double Plank_Width = Track_Diff / 3 * 5;//width of the rail track plank
const int Plank_Length = Plank_Diff / 10 * 3; //length of the rail track plank

int Count1 = 0;//Required in Camera Manipulation
double Coaster_Track_Height = 0.0;//The height of the roller coaster setup

double uCurrent = 0.0;//In Generating Splines.


int TotalCatmullPoints;//Total Catmull points generated
point* splinePoints;//required to generate splinepoints
point* SplinePt1; //1st spline point P1
point* SplinePt2; //2nd Splinr point P2
point* SplineTangent;//Tangent to the point
point* SplineNormal;//Normal to the point
point* SplineBiNormal;//BiNormal to the point

//Camera operations constants 
//http://run.usc.edu/cs420-s15/assignments/assign2/RollerCoasterVelocity.pdf
const double Up_Vector = Track_Diff / 10 * 8;//needed for computing the gluLookAt constants
const double Velocity = 6.0;//Camera displacement constant
const double DeltaT = Velocity / Increments;//time change

const double G = 9.78;//Gravitational Constant

//Implemented from the Class Slides.
//http://run.usc.edu/cs420-s15/lec08-splines/08-splines.pdf
point catmullRomPointCalc(point p0, point p1, point p2, point p3, double u)
{
 
      double Su = u * u;
	  double Cu = Su * u;
	  point result;
	  result.x  = (((Cu * (- p0.x + 3*p1.x - 3*p2.x + p3.x))*0.5) + ((Su * (2*p0.x - 5*p1.x + 4*p2.x - p3.x))* 0.5) + (((u * (p2.x - p0.x)) + 2*p1.x)* 0.5));
	  result.y  = (((Cu * (- p0.y + 3*p1.y - 3*p2.y + p3.y))*0.5) + ((Su * (2*p0.y - 5*p1.y + 4*p2.y - p3.y))* 0.5) + (((u * (p2.y - p0.y)) + 2*p1.y)* 0.5));
	  result.z  = (((Cu * (- p0.z + 3*p1.z - 3*p2.z + p3.z))*0.5) + ((Su * (2*p0.z - 5*p1.z + 4*p2.z - p3.z))* 0.5) + (((u * (p2.z - p0.z)) + 2*p1.z)* 0.5));
	  return result; 
	  	
}

// By differentiating catmull equation w.r.t 'u' from above function.
point catmullRomTangentCalc(point p0, point p1, point p2, point p3, double u) 
{
	  double Su = u * u;
	  point result;
	  
	result.x = 0.5 * ((-p0.x + p2.x) + (2*p0.x - 5*p1.x + 4*p2.x - p3.x) * 2 * u + (-p0.x + 3*p1.x - 3*p2.x + p3.x) * 3 * Su); 
	result.y = 0.5 * ((-p0.y + p2.y) + (2*p0.y - 5*p1.y + 4*p2.y - p3.y) * 2 * u + (-p0.y + 3*p1.y - 3*p2.y + p3.y) * 3 * Su); 
	result.z = 0.5 * ((-p0.z + p2.z) + (2*p0.z - 5*p1.z + 4*p2.z - p3.z) * 2 * u + (-p0.z + 3*p1.z - 3*p2.z + p3.z) * 3 * Su); 
    return result;
}



// From basic vector mathematics from the text book.
point normalize(point P) 
{
	float D;
	D = ((P.x * P.x)+ (P.y * P.y) + (P.z * P.z)); //Square Root of (x^2 + Y^2 + Z^2)
	float Denominator = sqrt(D);
	P.x = P.x / Denominator;
	P.y = P.y / Denominator;
	P.z = P.z / Denominator;
	return P;
}

//Simple Matrix Operation.
point crossProduct(point a, point b) 

{
	point c;
	c.x = a.y * b.z - a.z * b.y;
	c.y = a.z * b.x - a.x * b.z;
	c.z = a.x * b.y - a.y * b.x;
	return c;
}

//http://run.usc.edu/cs420-s15/assignments/assign2/RollerCoasterVelocity.pdf
// Implemented the formula given in the slides.
double computeNewVelocity(double uCurrent, point p, point t) 
{
  double Sr1,Sr2,Sr3, result;
  Sr1 = t.x * t.x;
  Sr2 = t.y * t.y;
  Sr3 = t.z * t.z;
  double Numerator = sqrt(2 * G * (Coaster_Track_Height - p.z));
  double Denominator = sqrt(Sr1 + Sr2 + Sr3);
  result = uCurrent + DeltaT * (Numerator / Denominator);
  return result;
}


//Given Helper Method
int loadSplines(char *argv) 
{
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;


  /* load the track file */
  fileList = fopen(argv, "r");
  if (fileList == NULL) {
    printf ("can't open file\n");
    exit(1);
  }
  
  /* stores the number of splines in a global variable */
  fscanf(fileList, "%d", &g_iNumOfSplines);

  g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

  /* reads through the spline files */
  for (j = 0; j < g_iNumOfSplines; j++) {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) {
      printf ("can't open file\n");
      exit(1);
    }

    /* gets length for spline file */
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    /* allocate memory for all the points */
    g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
    g_Splines[j].numControlPoints = iLength;

    /* saves the data to the struct */
    while (fscanf(fileSpline, "%lf %lf %lf", 
                  &g_Splines[j].points[i].x, 
                  &g_Splines[j].points[i].y, 
                  &g_Splines[j].points[i].z) != EOF) {
        i++;
    }
  }

  free(cName);

  return 0;
}

// Learned about Texture Mapping Programming from the following links.
//http://www.glprogramming.com/red/chapter09.html
//http://www.mbsoftworks.sk/index.php?page=tutorials&series=1&tutorial=9

void loadTexture(char *filename, GLuint textureID) 
{

	Pic* ImageFile = jpeg_read(filename, NULL);
	if (ImageFile == NULL) 
	{
    printf("Cannot Access image: %s.\n", filename);
    exit(1);
    }

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_REPEAT);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, ImageFile->nx, ImageFile->ny, GL_RGB, GL_UNSIGNED_BYTE, ImageFile->pix);
}


// Here we create the Splines from the track.txt file loaded from the main function.
// For every pair of 4 control points we create a series of splines by incrementing
// u in the steps of 0.001 i.e as we u varie from 0 to 1 it does so in terms of 0.001 step.
//Once created, we store the points to be rendered in the arrays firstSpline and secondSpline 
//Spline is drawn when we render the track by calling TrackDisplay() function.
void SplineCreate(spline* g_Splines) 
{
  int firstSpline = 0; //Initialization
  int numOfSplinePoints = 0;//Counter
  numOfSplinePoints = g_Splines[firstSpline].numControlPoints;//Load Number of Spline Points from the text file
  float uIncrement = 1.0 / Increments;//divide 1 into 1000 parts i.e 0.001,0.002,0.003....
  point R;
  R.x = 0.0; R.y = 0.0; R.z = -1.0; //Necessary for Normalization.
  
  splinePoints = new point[numOfSplinePoints * Increments];//Arrays to store Spline Points
  SplinePt1 = new point[numOfSplinePoints * Increments];//Load 1st point of the each catmull curve
														//Inializes each pair for part of the curve
  SplinePt2 = new point[numOfSplinePoints * Increments + 1];//end point of the catmull curve section
  SplineTangent = new point[numOfSplinePoints * Increments];// Tangent to the point
  SplineNormal = new point[numOfSplinePoints * Increments];// Normal of the point
  SplineBiNormal = new point[numOfSplinePoints * Increments];// BiNormal of the point
	  
  int I = 0;//for indexing the array
  for(int i = 1; i < numOfSplinePoints-2; i++) 
  {
    for(float u = 0.0; u < 1.0; u += uIncrement) 
	{
    
	  point p0, p1, p2, p3;// four control points needed for calulating catmull rom spline
      p0 = g_Splines[firstSpline].points[i];//loading the points to calculate the catmull
      p1 = g_Splines[firstSpline].points[i+1];
      p2 = g_Splines[firstSpline].points[i+2];
      p3 = g_Splines[firstSpline].points[i+3];

      
      point splinePoint = catmullRomPointCalc(p0,p1,p2,p3,u);//Returns the CatMull Rom Point
      splinePoints[I] = splinePoint;//Store it to an Array.
     if(splinePoint.z > Coaster_Track_Height) Coaster_Track_Height = splinePoint.z;//Keep track of the height of roller coaster

	 point splineTangent = catmullRomTangentCalc(p0,p1,p2,p3,u);//Calculate Tangent
      SplineTangent[I] = splineTangent;//Store it in Tangent Array
      
	  splineTangent = normalize(splineTangent); //Normalize the Tangent

      // calculate the spline normal and binormal. 
	  //http://run.usc.edu/cs420-s15/assignments/assign2/hw2Hints.pdf
      //steps for doing the calculation is well explained in class slides.
	  point splineNormal, splineBinormal;
      if(I == 0) {
        point T0 = splineTangent;
        splineNormal = normalize(crossProduct(T0, R));
        splineBinormal = normalize(crossProduct(T0, splineNormal));
      } else {
        point B0 = SplineBiNormal[I-1];
        point T1 = splineTangent;
        splineNormal = normalize(crossProduct(B0, T1));
        splineBinormal = normalize(crossProduct(T1, splineNormal));
      }
      SplineNormal[I] = splineNormal;
      SplineBiNormal[I] = splineBinormal;

  	  double Track_H_Midpoint = Track_Diff / 2.0;// required 
      
	  point firstSplinePoint;//Left face
      firstSplinePoint.x = splinePoint.x - 
        Track_H_Midpoint * splineNormal.x;
      firstSplinePoint.y = splinePoint.y - 
        Track_H_Midpoint * splineNormal.y;
      firstSplinePoint.z = splinePoint.z - 
        Track_H_Midpoint * splineNormal.z;
      SplinePt1[I] = firstSplinePoint;

	  point secondSplinePoint;//right face
      secondSplinePoint.x = splinePoint.x + 
        Track_H_Midpoint * splineNormal.x;
      secondSplinePoint.y = splinePoint.y + 
        Track_H_Midpoint * splineNormal.y;
      secondSplinePoint.z = splinePoint.z + 
        Track_H_Midpoint * splineNormal.z;
      SplinePt2[I] = secondSplinePoint;
	  
      I++;
    }
  }

  TotalCatmullPoints = I;
}

//Utility Functions given from Assignment 1.
void saveScreenshot (char *filename) 
{
  int i, j;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

//Implemented it for Assignment 1 and same being used here 
// to save screenshots.
void timeFunction(int value)
{
  switch(value)
  {
    case 0: 
      if(!stop_counter) break;
      char filename[40];
      sprintf(filename, "%00d.jpg", Count);
      saveScreenshot(filename);
      Count++;
      if(Count == 301) 
	  {stop_counter = false;
	  break;}
    default:break;
  }
  glutTimerFunc(15, timeFunction, 0);
}

//http://mbsoftworks.sk/index.php?page=tutorials&series=1&tutorial=13
//Takes 6 jpg images of 512 X 512 and map them along the walls of Scene.
// Volume of the Cube is of length 100 pixels.
void SceneBoxDisplay() {
  
  
   glGenTextures(1, &textures[0]);
  loadTexture("top.jpg", textures[0]);//up Sky
  glGenTextures(1, &textures[1]);
  loadTexture("bottom.jpg", textures[1]);//Ground 
  glGenTextures(1, &textures[2]);
  loadTexture("front.jpg", textures[2]);//Surroundings
  glGenTextures(1, &textures[3]);
  loadTexture("back.jpg", textures[3]);
  glGenTextures(1, &textures[4]);
  loadTexture("right.jpg", textures[4]);
  glGenTextures(1, &textures[5]);
  loadTexture("left.jpg", textures[5]);
  

  glEnable(GL_TEXTURE_2D);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
  glEnable(GL_COLOR_MATERIAL);

  // http://www.glprogramming.com/red/chapter09.html

  glBindTexture(GL_TEXTURE_2D,textures[0]);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0); glVertex3f(-100,+100,+100);
  glTexCoord2f(1,0); glVertex3f(+100,+100,+100);
  glTexCoord2f(1,1); glVertex3f(+100,-100,+100);
  glTexCoord2f(0,1); glVertex3f(-100,-100,+100);
  glEnd();
  
  glBindTexture(GL_TEXTURE_2D,textures[1]);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0); glVertex3f(-100,-100,-100);
  glTexCoord2f(1,0); glVertex3f(+100,-100,-100);
  glTexCoord2f(1,1); glVertex3f(+100,+100,-100);
  glTexCoord2f(0,1); glVertex3f(-100,+100,-100);
  glEnd();
  
  glBindTexture(GL_TEXTURE_2D,textures[2]);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0); glVertex3f(+100,-100,+100);
  glTexCoord2f(1,0); glVertex3f(+100,+100,+100);
  glTexCoord2f(1,1); glVertex3f(+100,+100,-100);
  glTexCoord2f(0,1); glVertex3f(+100,-100,-100);
  glEnd();
  
  glBindTexture(GL_TEXTURE_2D,textures[3]);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0); glVertex3f(-100,+100,+100);
  glTexCoord2f(1,0); glVertex3f(-100,-100,+100);
  glTexCoord2f(1,1); glVertex3f(-100,-100,-100);
  glTexCoord2f(0,1); glVertex3f(-100,+100,-100);
  glEnd();
  
  glBindTexture(GL_TEXTURE_2D,textures[4]);
  glBegin(GL_QUADS);
  glTexCoord2f(1,0); glVertex3f(+100,-100,+100);
  glTexCoord2f(1,1); glVertex3f(+100,-100,-100);
  glTexCoord2f(0,1); glVertex3f(-100,-100,-100);
  glTexCoord2f(0,0); glVertex3f(-100,-100,+100);
  glEnd();
  
  glBindTexture(GL_TEXTURE_2D,textures[5]);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0); glVertex3f(+100,+100,+100);
  glTexCoord2f(1,0); glVertex3f(-100,+100,+100);
  glTexCoord2f(1,1); glVertex3f(-100,+100,-100);
  glTexCoord2f(0,1); glVertex3f(+100,+100,-100);
  glEnd();
  
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_TEXTURE_2D);
 
}


//Displays the Coaster Track consisting of 2 Curved tracks.
//Each track is a 3D cuboid extending along the path. 
void TrackDisplay() 
{

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
    
  
for(int i = 0; i < g_iNumOfSplines; i++) 
  {
    for(int j = 1; j < TotalCatmullPoints; j++) 
	{
         double f = 0.009;//thickness of the rail track

      point p1 = SplinePt1[j];//Loading the 1st Spline Point
      point n1 = SplineNormal[j];//its Normal
      point b1 = SplineBiNormal[j];//Its BiNormal
      
      point p2 = SplinePt1[j + 1];//Loading the 2nd point
      point n2 = SplineNormal[j + 1];
      point b2 = SplineBiNormal[j + 1];

	  glEnable(GL_COLOR_MATERIAL);
		glColor3f(1.0,0.0,0.0);
		glBegin(GL_QUADS);
     
		//Display the left rail track
glVertex3d(p1.x+f*(n1.x+b1.x), p1.y+f*(n1.y+b1.y), p1.z+f*(n1.z+b1.z));
glVertex3d(p2.x+f*(n2.x+b2.x), p2.y+f*(n2.y+b2.y), p2.z+f*(n2.z+b2.z));
glVertex3d(p2.x+f*(b2.x-n2.x), p2.y+f*(b2.y-n2.y), p2.z+f*(b2.z-n2.z));
glVertex3d(p1.x+f*(b1.x-n1.x), p1.y+f*(b1.y-n1.y), p1.z+f*(b1.z-n1.z));
      
glVertex3d(p1.x+f*(n1.x-b1.x), p1.y+f*(n1.y-b1.y), p1.z+f*(n1.z-b1.z));
glVertex3d(p2.x+f*(n2.x-b2.x), p2.y+f*(n2.y-b2.y), p2.z+f*(n2.z-b2.z));
glVertex3d(p2.x+f*(-b2.x-n2.x), p2.y+f*(-b2.y-n2.y), p2.z+f*(-b2.z-n2.z));
glVertex3d(p1.x+f*(-b1.x-n1.x), p1.y+f*(-b1.y-n1.y), p1.z+f*(-b1.z-n1.z));
      
glVertex3d(p1.x+f*(-n1.x+b1.x), p1.y+f*(-n1.y+b1.y), p1.z+f*(-n1.z+b1.z));
glVertex3d(p2.x+f*(-n2.x+b2.x), p2.y+f*(-n2.y+b2.y), p2.z+f*(-n2.z+b2.z));
glVertex3d(p2.x+f*(-b2.x-n2.x), p2.y+f*(-b2.y-n2.y), p2.z+f*(-b2.z-n2.z));
glVertex3d(p1.x+f*(-b1.x-n1.x), p1.y+f*(-b1.y-n1.y), p1.z+f*(-b1.z-n1.z));

glVertex3d(p1.x+f*(n1.x+b1.x), p1.y+f*(n1.y+b1.y), p1.z+f*(n1.z+b1.z));
glVertex3d(p2.x+f*(n2.x+b2.x), p2.y+f*(n2.y+b2.y), p2.z+f*(n2.z+b2.z));
glVertex3d(p2.x+f*(n2.x-b2.x), p2.y+f*(n2.y-b2.y), p2.z+f*(n2.z-b2.z));
glVertex3d(p1.x+f*(n1.x-b1.x), p1.y+f*(n1.y-b1.y), p1.z+f*(n1.z-b1.z));

      p1 = SplinePt2[j];
      p2 = SplinePt2[j + 1];

	//display the right rail track
glVertex3d(p1.x+f*(n1.x+b1.x), p1.y+f*(n1.y+b1.y), p1.z+f*(n1.z+b1.z));
glVertex3d(p2.x+f*(n2.x+b2.x), p2.y+f*(n2.y+b2.y), p2.z+f*(n2.z+b2.z));
glVertex3d(p2.x+f*(b2.x-n2.x), p2.y+f*(b2.y-n2.y), p2.z+f*(b2.z-n2.z));
glVertex3d(p1.x+f*(b1.x-n1.x), p1.y+f*(b1.y-n1.y), p1.z+f*(b1.z-n1.z));
      
glVertex3d(p1.x+f*(n1.x-b1.x), p1.y+f*(n1.y-b1.y), p1.z+f*(n1.z-b1.z));
glVertex3d(p2.x+f*(n2.x-b2.x), p2.y+f*(n2.y-b2.y), p2.z+f*(n2.z-b2.z));
glVertex3d(p2.x+f*(-b2.x-n2.x), p2.y+f*(-b2.y-n2.y), p2.z+f*(-b2.z-n2.z));
glVertex3d(p1.x+f*(-b1.x-n1.x), p1.y+f*(-b1.y-n1.y), p1.z+f*(-b1.z-n1.z));
      
glVertex3d(p1.x+f*(-n1.x+b1.x), p1.y+f*(-n1.y+b1.y), p1.z+f*(-n1.z+b1.z));
glVertex3d(p2.x+f*(-n2.x+b2.x), p2.y+f*(-n2.y+b2.y), p2.z+f*(-n2.z+b2.z));
glVertex3d(p2.x+f*(-b2.x-n2.x), p2.y+f*(-b2.y-n2.y), p2.z+f*(-b2.z-n2.z));
glVertex3d(p1.x+f*(-b1.x-n1.x), p1.y+f*(-b1.y-n1.y), p1.z+f*(-b1.z-n1.z));

glVertex3d(p1.x+f*(n1.x+b1.x), p1.y+f*(n1.y+b1.y), p1.z+f*(n1.z+b1.z));
glVertex3d(p2.x+f*(n2.x+b2.x), p2.y+f*(n2.y+b2.y), p2.z+f*(n2.z+b2.z));
glVertex3d(p2.x+f*(n2.x-b2.x), p2.y+f*(n2.y-b2.y), p2.z+f*(n2.z-b2.z));
glVertex3d(p1.x+f*(n1.x-b1.x), p1.y+f*(n1.y-b1.y), p1.z+f*(n1.z-b1.z));

  glEnd();
  glDisable(GL_COLOR_MATERIAL);
  glColor3f(1,1,1);
    }

  }
  
}

//The Track Horizontal Planks along the path.
//Each is a 3D Box with 6 faces and texture mapped.
void RailwayPlanksDisplay() 
{

    glGenTextures(1, &textures[6]);
    loadTexture("rail.jpg", textures[6]);//The Texture for the track Planks
    
    glEnable(GL_TEXTURE_2D);

  int count = 0;//decides when to place a plank on the track.
  for(int i = 0; i < g_iNumOfSplines; i++)
 {
    for(int j = 0; j < TotalCatmullPoints; j++)
 {
      if(count == Plank_Diff)
 {
        count = 0;
        point n = SplineNormal[j];
        point b = SplineBiNormal[j];
        point p = splinePoints[j];
        point np = splinePoints[j + Plank_Length];
        double h = Plank_Thick;
        double w = Plank_Width;
        float f = 0.01;// rail track thickness

        p.x = p.x - f * b.x;
        p.y = p.y - f * b.y;
        p.z = p.z - f * b.z;
        np.x = np.x - f * b.x;
        np.y = np.y - f * b.y;
        np.z = np.z - f * b.z;

        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
        glEnable(GL_COLOR_MATERIAL);

        glBindTexture(GL_TEXTURE_2D, textures[6]);
        glBegin(GL_QUADS);
        
	  glTexCoord2f(0.0, 0.0); 
    glVertex3d(p.x-w/2.0*n.x, p.y-w/2.0*n.y, p.z-w/2.0*n.z);
        glTexCoord2f(0.0, 1.0); 
    glVertex3d(p.x-w/2.0*n.x-h*b.x, p.y-w/2.0*n.y-h*b.y, p.z-w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 1.0); 
    glVertex3d(p.x+w/2.0*n.x-h*b.x, p.y+w/2.0*n.y-h*b.y, p.z+w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 0.0); 
    glVertex3d(p.x+w/2.0*n.x, p.y+w/2.0*n.y, p.z+w/2.0*n.z);
        
		glTexCoord2f(0.0, 0.0); 
    glVertex3d(np.x-w/2.0*n.x, np.y-w/2.0*n.y, np.z-w/2.0*n.z);
        glTexCoord2f(0.0, 1.0); 
   glVertex3d(np.x-w/2.0*n.x-h*b.x, np.y-w/2.0*n.y-h*b.y, np.z-w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 1.0); 
   glVertex3d(np.x+w/2.0*n.x-h*b.x, np.y+w/2.0*n.y-h*b.y, np.z+w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 0.0); 
   glVertex3d(np.x+w/2.0*n.x, np.y+w/2.0*n.y, np.z+w/2.0*n.z);
        
		glTexCoord2f(0.0, 0.0); 
   glVertex3d(p.x-w/2.0*n.x-h*b.x, p.y-w/2.0*n.y-h*b.y, p.z-w/2.0*n.z-h*b.z);
        glTexCoord2f(0.0, 1.0); 
   glVertex3d(np.x-w/2.0*n.x-h*b.x, np.y-w/2.0*n.y-h*b.y, np.z-w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 1.0); 
   glVertex3d(np.x+w/2.0*n.x-h*b.x, np.y+w/2.0*n.y-h*b.y, np.z+w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 0.0); 
   glVertex3d(p.x+w/2.0*n.x-h*b.x, p.y+w/2.0*n.y-h*b.y, p.z+w/2.0*n.z-h*b.z);
        
		glTexCoord2f(0.0, 0.0); 
   glVertex3d(p.x-w/2.0*n.x, p.y-w/2.0*n.y, p.z-w/2.0*n.z);
        glTexCoord2f(0.0, 1.0); 
   glVertex3d(np.x-w/2.0*n.x, np.y-w/2.0*n.y, np.z-w/2.0*n.z);
        glTexCoord2f(1.0, 1.0); 
   glVertex3d(np.x+w/2.0*n.x, np.y+w/2.0*n.y, np.z+w/2.0*n.z);
        glTexCoord2f(1.0, 0.0); 
   glVertex3d(p.x+w/2.0*n.x, p.y+w/2.0*n.y, p.z+w/2.0*n.z);
        
		glTexCoord2f(0.0, 0.0); 
    glVertex3d(p.x+w/2.0*n.x, p.y+w/2.0*n.y, p.z+w/2.0*n.z);
        glTexCoord2f(0.0, 1.0); 
    glVertex3d(np.x+w/2.0*n.x, np.y+w/2.0*n.y, np.z+w/2.0*n.z);
        glTexCoord2f(1.0, 1.0); 
   glVertex3d(np.x+w/2.0*n.x-h*b.x, np.y+w/2.0*n.y-h*b.y, np.z+w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 0.0); 
   glVertex3d(p.x+w/2.0*n.x-h*b.x, p.y+w/2.0*n.y-h*b.y, p.z+w/2.0*n.z-h*b.z);
    
		glTexCoord2f(0.0, 0.0); 
   glVertex3d(p.x-w/2.0*n.x, p.y-w/2.0*n.y, p.z-w/2.0*n.z);
        glTexCoord2f(0.0, 1.0); 
   glVertex3d(np.x-w/2.0*n.x, np.y-w/2.0*n.y, np.z-w/2.0*n.z);
        glTexCoord2f(1.0, 1.0); 
   glVertex3d(np.x-w/2.0*n.x-h*b.x, np.y-w/2.0*n.y-h*b.y, np.z-w/2.0*n.z-h*b.z);
        glTexCoord2f(1.0, 0.0); 
    glVertex3d(p.x-w/2.0*n.x-h*b.x, p.y-w/2.0*n.y-h*b.y, p.z-w/2.0*n.z-h*b.z);
        glEnd();

        glDisable(GL_COLOR_MATERIAL);
		
    
  }
      count++;

  }

  }

  glDisable(GL_TEXTURE_2D);
}

//Extra Credit: Since the track is suspended in the sky it was reasonable to use smoky fog 
// feature of openGL for good effects. 
void FogUp()
{
 GLfloat fogcolour[4]={1.0,1.0,1.0,0.0};         
 GLuint	fogMode[]= { GL_EXP2, GL_LINEAR };	// Storage For Three Types Of Fog
 GLfloat	fogColor[4] = {0.6,0.6,0.6, 0.9};		// Fog Color

    
 	glFogi(GL_FOG_MODE, GL_LINEAR);			// Fog Mode
	glFogfv(GL_FOG_COLOR, fogColor);					// Set Fog Color
	glHint(GL_FOG_HINT, GL_NICEST);					// Fog Hint Value
	glFogf(GL_FOG_START, 50.0f);							// Fog Start Depth
	glFogf(GL_FOG_END, 120.0f);							// Fog End Depth
	glEnable(GL_FOG);									// Enables GL_FOG

}

//http://www.tomdalling.com/blog/modern-opengl/06-diffuse-point-lighting/
//Extra Credit: Simple light settings
void LightUp() {

	GLfloat LightAmbient[]=		{ 0.5, 0.5, 0.5, 1.0 };
    GLfloat LightDiffuse[]=		{ 1.0, 1.0, 1.0, 1.0 };
    GLfloat LightPosition[]=	{ 0.0, 0.0, 0.0, 1.0 };

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT0, GL_POSITION,LightPosition);	// Position The Light
	glEnable(GL_LIGHT1);								

}

//http://run.usc.edu/cs420-s15/assignments/assign2/RollerCoasterVelocity.pdf
//Camera changes its speed at a rate delta time.
//Has been implemented using the explaination given in tips slides for the assignment 2.
void CameraSetUp() 
{
  point Point, Tangent, Normal, BiNrml_Up, Eye, Center;
  
  Point = splinePoints[(int)(Count1 + Increments * count)];
  Tangent = normalize(SplineTangent[(int)(Count1 + Increments * count)]);
  Normal = SplineNormal[(int)(Count1 + Increments * count)];
  BiNrml_Up = SplineBiNormal[(int)(Count1 + Increments * count)];

  Eye.x = Point.x + Up_Vector * BiNrml_Up.x;
  Eye.y = Point.y + Up_Vector * BiNrml_Up.y;
  Eye.z = Point.z + Up_Vector * BiNrml_Up.z;

  Center.x = Point.x + Up_Vector * BiNrml_Up.x + Tangent.x;
  Center.y = Point.y + Up_Vector * BiNrml_Up.y + Tangent.y;
  Center.z = Point.z + Up_Vector * BiNrml_Up.z + Tangent.z;

  //places the camera along the track, use Binormal for up vector not the Normal 
  gluLookAt(Eye.x, Eye.y, Eye.z, Center.x, Center.y, Center.z, BiNrml_Up.x, BiNrml_Up.y, BiNrml_Up.z);

  Tangent = SplineTangent[(int)(Count1 + Increments * count)];

  double uNew = computeNewVelocity(count, Point, Tangent);
     
     if(uNew - count < Velocity / Increments)
     		 uNew = Velocity / Increments + count;
     
	 if(uNew >= 1.0) 
	 {
       Count1 += Increments;
       uNew = 0.0;
     }
     
	 if(Count1 >= TotalCatmullPoints) 
	 {
     Count1 = 0;
     }
     count = uNew;
}


void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (FogUpStart)FogUp();//If F pressed then render Fog Effects.

  glLoadIdentity();//Load the Identity Matrix for all the Transformations
  
  glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
  glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);

  CameraSetUp();//First set up the camera to enjoy the ride.
  SceneBoxDisplay();//Render the Scene
  RailwayPlanksDisplay();// Render the Track Planks
  TrackDisplay();//Now thr Tracks on which Coaster drives/
  glFlush();
  glutSwapBuffers();
}


void doIdle() 
{
  glutPostRedisplay();
}

void reshape(int width, int height){
  GLfloat aspect = (GLfloat) width / (GLfloat) height;
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, aspect, 0.01, 1000.0);//angle of view = 60.0
  glMatrixMode(GL_MODELVIEW);
}

//Implemented in the similar manner as Assignment 1.
void mousedrag(int x, int y) {
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
   
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1];
        g_vLandRotate[1] += vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y) {
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y) {

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
  case 'q': case 'Q': //Quits
    exit(0);
    break;

  case 'f': case 'F':
	        FogUpStart = true; // Starts Fog Effect
            break;

  case 'r': case 'R':
	        FogUpStart = false;
			break;

  default: break;
  }
  glutPostRedisplay();
}

void menufunc(int value) {
  switch (value)
  {
    case 0:
      exit(0);
      break;
  }
}



void myInit() 
{
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	LightUp();//Setup the light settings
	SplineCreate(g_Splines);//Create the spline points using coordinates from the track file
	
}

int main (int argc, char ** argv) 
{

	if (argc<2)
  {  
  printf ("usage: %s <trackfile>\n", argv[0]);
  exit(0);
  }

  loadSplines(argv[1]);//load the track file

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(640,480);
  glutCreateWindow("Avinash Akka: Roller Coaster");

  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit",0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutKeyboardFunc(keyboard);

  glutMotionFunc(mousedrag); 
  glutPassiveMotionFunc(mouseidle); 
  glutMouseFunc(mousebutton);

  glutReshapeFunc(reshape);//reshape function callback
  glutIdleFunc(doIdle);//idle function callback
  glutDisplayFunc(display);//display function callback
  glutTimerFunc(15, timeFunction, 0);//timer function required for screenshot saving

  myInit();//initialization function
  glutMainLoop();
  return 0;
}
