
Avinash 

CSCI 420: Assignment 2: Roller Coaster Simulation


1. The Code has been written in C Language and has been compiled   
   and executed using Microsoft Visual Studio 2010 on a Windows 8 
   Operating System.

2. The Utility functions to Save Screenshots and load splines has    
   been used unchanged.

3. The function SplineCreate() in called when initialization    
   which creates the spline points and saves them in the array to 
   be rendered later when called in display callback.

4. The timer function implementation has been referenced from  
   www.glprogramming.com which has been used unchanged from 
   Assignment 1.

5. The View Box or the Sky Box is a cube of dimension 100X100X100 
   with its inside walls texture mapped.The Track has been 
   rendered inside this box.

6. Load the goodRide spline for a good view of the surrounding.

7. Place the texture images in the root folder where assign2.cpp 
   is placed.

TASK COMPLETED

LEVEL 1: Splines have been generated and implemented by using 
         Catmull-Rom Spline Function.

LEVEL 2: Ground has been textured mapped as asked.

LEVEL 3: Sky has been textured mapped

LEVEL 4: Camera has been setup to manouver with the ride with 
         help of the slides from the class noted.

LEVEL 5: Double tracks have been rendered as asked.Each track is  
         a 3D cuboid that extends along the track.
	    Was implemented using: http://run.usc.edu/cs420-
         s15/assignments/assign2/csci480_assign2_crossSection.pdf
 
KEYS DESCRIPTION

Press q or Q : To Exit.

Press f or F : To Render Smoky Fog.

Use Mouse keys to rotate the camera view.


EXTRA CREDITS IMPLEMENTATION 

1.Rendered double track in 3D.

2.OpenGL Lighting has been used.

3.Rendered the environment in a prettier manner.

4.Rail track cross planks have been rendered in 3D.

5.Each cross Plank is texture mapped to look like a metal plate.

6.Camera moves to make it physically realistic in terms of 
  gravity.

7.Physically realistic equation of updating the u in camera 
  movement.

8.Smoky Fog using OpenGL Fog functions has been used to show the 
  fog effect.







