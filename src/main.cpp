/**
 * Author:	Andrew Robert Owens
 * Email:	arowens [at] ucalgary.ca
 * Date:	January, 2017
 * Course:	CPSC 587/687 Fundamental of Computer Animation
 * Organization: University of Calgary
 *
 * Copyright (c) 2017 - Please give credit to the author.
 *
 * File:	main.cpp
 *
 * Summary:
 *
 * This is a (very) basic program to
 * 1) load shaders from external files, and make a shader program
 * 2) make Vertex Array Object and Vertex Buffer Object for the quad
 *
 * take a look at the following sites for further readings:
 * opengl-tutorial.org -> The first triangle (New OpenGL, great start)
 * antongerdelan.net -> shaders pipeline explained
 * ogldev.atspace.co.uk -> good resource
 */

#include <iostream>
#include <cmath>
#include <chrono>
#include <limits>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "ShaderTools.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "OpenGLMatrixTools.h"
#include "Camera.h"

//==================== GLOBAL VARIABLES ====================//
/*	Put here for simplicity. Feel free to restructure into
*	appropriate classes or abstractions.
*/

// Drawing Program
GLuint basicProgramID;

// Data needed for Quad
GLuint vaoID;
GLuint vertBufferID;
Mat4f M;

// Data needed for Line 
GLuint line_vaoID;
GLuint line_vertBufferID;
Mat4f line_M;

// Only one camera so only one veiw and perspective matrix are needed.
Mat4f V;
Mat4f P;

// Only one thing is rendered at a time, so only need one MVP
// When drawing different objects, update M and MVP = M * V * P
Mat4f MVP;

// Camera and veiwing Stuff
Camera camera;
int g_moveUpDown = 0;
int g_moveLeftRight = 0;
int g_moveBackForward = 0;
int g_rotateLeftRight = 0;
int g_rotateUpDown = 0;
int g_rotateRoll = 0;
float g_rotationSpeed = 0.015625;
float g_panningSpeed = 2.5;
bool g_cursorLocked;
float g_cursorX, g_cursorY;

bool g_play = false;

int WIN_WIDTH = 800, WIN_HEIGHT = 600;
int FB_WIDTH = 800, FB_HEIGHT = 600;
float WIN_FOV = 60;
float WIN_NEAR = 0.01;
float WIN_FAR = 1000;

//==================== FUNCTION DECLARATIONS ====================//
void render();
void resizeFunc();
void init();
void generateIDs();
void deleteIDs();
void setupVAO();
void loadCartGeometryToGPU();
void reloadProjectionMatrix();
void loadModelViewMatrix();
void setupModelViewProjectionTransform();

void windowSetSizeFunc();
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowSetSizeFunc(GLFWwindow *window, int width, int height);
void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height);
void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void animateCart(float t);
void moveCamera();
void reloadMVPUniform();
void reloadColorUniform(float r, float g, float b);
std::string GL_ERROR();
int main(int, char **);

//==================== FUNCTION DEFINITIONS ====================//

#define gravity 9
float highPoint = -50.f;
float velocity = 5.f;
float dt = 0.077;
int counter = -1;
Vec3f actualPos;

bool firstPerson = false;

float decelVel = -1.f;
bool finalLoop = false;

Vec3f frenetN;
Vec3f frenetB;
Vec3f frenetT;

std::vector<Vec3f> path;
std::vector<Vec3f> track;

/*
int numCurves = 2;
std::vector<int> degrees = {1, 3};
std::vector<Vec3f> controlPoints = {
		Vec3f(0.f, 0.f, 0.f), Vec3f(-50.f, 0.f, 0.f),
		Vec3f(-100.f, 0.f, 0.f), Vec3f(-100.f, 500.f, 0.f), Vec3f(-200.f, 100.f, 0.f)

	};
*/

int numCurves = 4;
std::vector<int> degrees = {3,2,3,3};
std::vector<Vec3f> controlPoints = {
		Vec3f(-100.f, -50.f, 100.f), Vec3f(-200.f, -50.f, 250.f), Vec3f(100.f, -100.f, 0.f), Vec3f(100.f, 0.f, -50.f),
		Vec3f(100.f, 100.f, -100.f), Vec3f(0.f, 100.f, 0.f),
		Vec3f(-100.f, 100.f, 100.f), Vec3f(-100.f, 0.f, 0.f), Vec3f(-50.f, 0.f, -20.f),
		Vec3f(0.f, 0.f, -40.f), Vec3f(0.f, -50.f, -50.f), Vec3f(-100.f, -50.f, 100.f)
	};


void render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Use our shader
  glUseProgram(basicProgramID);

  // ===== DRAW QUAD ====== //
  MVP = P * V * M;
  reloadMVPUniform();
  reloadColorUniform(1, 0, 1);

  // Use VAO that holds buffer bindings
  // and attribute config of buffers
  glBindVertexArray(vaoID);
  // Draw Quads, start at vertex 0, draw 4 of them (for a quad)
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

  // ==== DRAW LINE ===== //
  MVP = P * V * line_M;
  reloadMVPUniform();

  reloadColorUniform(0, 1, 1);

  // Use VAO that holds buffer bindings
  // and attribute config of buffers
  glBindVertexArray(line_vaoID);
  // Draw lines
  glDrawArrays(GL_TRIANGLE_STRIP, 0, track.size());
  
}

float findDist(Vec3f p1, Vec3f p2){
		float distX = p1.x() - p2.x();
		float distY = p1.y() - p2.y();
		float distZ = p1.z() - p2.z();
		return(sqrt((distX * distX) + (distY * distY) + (distZ * distZ)));
}

Vec3f cross(Vec3f a, Vec3f b){
	float productX = (a.y() * b.z()) - (a.z() * b.y());
	float productY = (a.z() * b.x()) - (a.x() * b.z());
	float productZ = (a.x() * b.y()) - (a.y() * b.x());
	return Vec3f(productX, productY, productZ);
}

float vectorLength(Vec3f vec){
	return sqrt( (vec.x()*vec.x()) + (vec.y()*vec.y()) + (vec.z()*vec.z()) );
}

Vec3f normalize(Vec3f vec){
	return ( vec / vectorLength(vec) );
}

void buildFrenet(Vec3f currPos, float disp){
	if (disp == 0.f)
		disp = 0.1f;

	int sizeOfpath = path.size();

	float distanceToTravel = disp * dt;
	int nextPoint = counter;
	while(distanceToTravel > 0){
		nextPoint++;
		if (nextPoint >= sizeOfpath)
			nextPoint = nextPoint % sizeOfpath;
		distanceToTravel -= findDist(path[nextPoint], currPos);
	}
	Vec3f posPlus1 = (1.f-distanceToTravel)*path[nextPoint] + distanceToTravel*currPos;

	distanceToTravel = disp * dt;
	nextPoint = counter;
	while(distanceToTravel > 0){
		nextPoint--;
		if (nextPoint < 0)
			nextPoint += sizeOfpath;
		distanceToTravel -= findDist(path[nextPoint], currPos);
	}
	Vec3f posMinus1 = (1.f-distanceToTravel)*path[nextPoint] + distanceToTravel*currPos;

	/*float radius = ((disp * dt) * (disp * dt)) / vectorLength(posPlus1 - posMinus1);
	float perpForce = (disp * disp) / radius;
	Vec3f perpAccel = (normalize(posPlus1 - posMinus1)) * perpForce;*/

	Vec3f xVec = posPlus1 - 2*currPos + posMinus1;
	float x = 0.5 * vectorLength(xVec);
	Vec3f cVec = posPlus1 - posMinus1;
	float c = 0.5 * vectorLength(cVec);
	
	frenetT = normalize(posPlus1 - posMinus1);

	if (vectorLength(cross( (posPlus1 - currPos) , (currPos - posMinus1))) < 0.00001){
		frenetN = normalize(Vec3f(frenetT.z(), 0.f, -frenetT.x()));
	} else {
		Vec3f perpAccel = (disp * disp) * (xVec / ((x*x) + (c*c)));
		frenetN = normalize(perpAccel) - normalize(Vec3f(0.f, gravity, 0.f));
	}
	frenetB = normalize(cross(frenetT, frenetN));
}

void animateCart() {
	int sizeOfpath = path.size();
	float distanceToTravel = velocity * dt;
	int nextPoint = 0;

	while(distanceToTravel > 0){
		counter++;
		nextPoint = counter + 1;

		if (counter >= sizeOfpath)
			counter = counter % sizeOfpath;
		if (nextPoint >= sizeOfpath)
			nextPoint = nextPoint % sizeOfpath;

		distanceToTravel -= findDist(path[nextPoint], path[counter]);
	}

	actualPos = (1.f-distanceToTravel)*path[nextPoint] + distanceToTravel*path[counter];
	if (highPoint < actualPos.y())
		highPoint = actualPos.y();

	float newEk = gravity * (highPoint - actualPos.y());
 	velocity = std::max( float(sqrt((2.f*newEk))) , 5.f);

  float decelPoint = (sizeOfpath * 0.05) * 19;
	if (finalLoop && (counter > decelPoint)){
		float cartDist = (sizeOfpath - 2) - counter;
		float decelDist = sizeOfpath - decelPoint;
		if (decelVel < velocity)
			decelVel = velocity;
		velocity = decelVel * (cartDist / decelDist);

		if (velocity == 0.f){
			highPoint = actualPos.y();
			decelVel = -1.f;
		}
	}

	buildFrenet(actualPos, velocity);

	/*M[0] = frenetB.x(); M[1] = frenetN.x(); M[2] = frenetT.x(); M[3] = (actualPos + frenetB).x();
	M[4] = frenetB.y(); M[5] = frenetN.y(); M[6] = frenetT.y(); M[7] = (actualPos + frenetB).y();
	M[8] = frenetB.z(); M[9] = frenetN.z(); M[10] = frenetT.z(); M[11] = (actualPos + frenetB).z();
	M[12] = 0.f; M[13] = 0.f; M[14] = 0.f; M[15] = 1.f;*/
  M = TranslateMatrix(actualPos + frenetB);
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

void loadCartGeometryToGPU() {
  // Just basic layout of floats
  std::vector<Vec3f> CartPoints;
  CartPoints.push_back(Vec3f(-2.5, -2.5, 0));
  CartPoints.push_back(Vec3f(-2.5, 2.5, 0));

  CartPoints.push_back(Vec3f(2.5, -2.5, 0));
  CartPoints.push_back(Vec3f(2.5, 2.5, 0));

  CartPoints.push_back(Vec3f(2.5, -2.5, -5));
  CartPoints.push_back(Vec3f(2.5, 2.5, -5));

  CartPoints.push_back(Vec3f(-2.5, -2.5, -5));
  CartPoints.push_back(Vec3f(-2.5, 2.5, -5));

  CartPoints.push_back(Vec3f(-2.5, -2.5, 0));
  CartPoints.push_back(Vec3f(-2.5, 2.5, 0));

  glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Vec3f) * 10, // byte size of Vec3f, 4 of them
               CartPoints.data(),      // pointer (Vec3f*) to contents of CartPoints
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer
}

void findPath(){
	int displacement = 0;
	Vec3f c0;
	Vec3f c1;
	Vec3f c2;
	Vec3f c3;
	for(int i = 0; i < numCurves; i++){
		switch (degrees[i]){
			case 1 :
				c0 = controlPoints[0 + displacement];
				c1 = controlPoints[1 + displacement];
				for(float u = 0.f; u<=1.f; u+=0.001f){
					float v = 1.f-u;
					path.push_back(c0*v + c1*u);
				}
				break;
			case 2 :
				c0 = controlPoints[0 + displacement];
				c1 = controlPoints[1 + displacement];
				c2 = controlPoints[2 + displacement];
				for(float u = 0.f; u<=1.f; u+=0.001f){
					float v = 1.f-u;
					path.push_back(c0*v*v + 2.f*c1*u*v + c2*u*u);
				}
				break;
			case 3 :
				c0 = controlPoints[0 + displacement];
				c1 = controlPoints[1 + displacement];
				c2 = controlPoints[2 + displacement];
				c3 = controlPoints[3 + displacement];
				for(float u = 0.f; u<=1.f; u+=0.001f){
					float v = 1.f-u;
					path.push_back(c0*v*v*v + 3.f*c1*u*v*v + 3.f*c2*u*u*v + c3*u*u*u);
				}
				break;
		}
		displacement += degrees[i];
	}
}

void loadLineGeometryToGPU() {
  // read file for control points data identifying:
  	//number of bezier curves
		//their degrees
		//the actual control values
	//then use the data above to get the intermediate values of the bezier curves for each degree
	findPath();
	float disp = 0.1f;
	for(unsigned i = 0; i < path.size(); i++){
		buildFrenet(path[i], disp);
		track.push_back(path[i] + frenetN);
		track.push_back(path[i] - frenetN);
	}

  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Vec3f) * track.size(), // byte size of Vec3f
               track.data(),      // pointer (Vec3f*) to contents of track
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer
}

void setupVAO() {
  glBindVertexArray(vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
  glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
  glVertexAttribPointer(0,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
                        );

  glBindVertexArray(line_vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glVertexAttribPointer(0,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
                        );

  glBindVertexArray(0); // reset to default
}

void reloadProjectionMatrix() {
  // Perspective Only

  // field of view angle 60 degrees
  // window aspect ratio
  // near Z plane > 0
  // far Z plane

  P = PerspectiveProjection(WIN_FOV, // FOV
                            static_cast<float>(WIN_WIDTH) /
                                WIN_HEIGHT, // Aspect
                            WIN_NEAR,       // near plane
                            WIN_FAR);       // far plane depth
}

void loadModelViewMatrix() {
  M = IdentityMatrix();
  line_M = IdentityMatrix();
  // view doesn't change, but if it did you would use this
  V = camera.lookatMatrix();
}

void reloadViewMatrix() { V = camera.lookatMatrix(); }

void setupModelViewProjectionTransform() {
  MVP = P * V * M; // transforms vertices from right to left (odd huh?)
}

void reloadMVPUniform() {
  GLint id = glGetUniformLocation(basicProgramID, "MVP");

  glUseProgram(basicProgramID);
  glUniformMatrix4fv(id,        // ID
                     1,         // only 1 matrix
                     GL_TRUE,   // transpose matrix, Mat4f is row major
                     MVP.data() // pointer to data in Mat4f
                     );
}

void reloadColorUniform(float r, float g, float b) {
  GLint id = glGetUniformLocation(basicProgramID, "inputColor");

  glUseProgram(basicProgramID);
  glUniform3f(id, // ID in basic_vs.glsl
              r, g, b);
}

void generateIDs() {
  // shader ID from OpenGL
  std::string vsSource = loadShaderStringfromFile("./shaders/basic_vs.glsl");
  std::string fsSource = loadShaderStringfromFile("./shaders/basic_fs.glsl");
  basicProgramID = CreateShaderProgram(vsSource, fsSource);

  // VAO and buffer IDs given from OpenGL
  glGenVertexArrays(1, &vaoID);
  glGenBuffers(1, &vertBufferID);
  glGenVertexArrays(1, &line_vaoID);
  glGenBuffers(1, &line_vertBufferID);
}

void deleteIDs() {
  glDeleteProgram(basicProgramID);

  glDeleteVertexArrays(1, &vaoID);
  glDeleteBuffers(1, &vertBufferID);
  glDeleteVertexArrays(1, &line_vaoID);
  glDeleteBuffers(1, &line_vertBufferID);
}

void init() {
  glEnable(GL_DEPTH_TEST);
  glPointSize(50);

  camera = Camera(Vec3f{0, 0, 350}, Vec3f{0, 0, -1}, Vec3f{0, 1, 0});

  // SETUP SHADERS, BUFFERS, VAOs

  generateIDs();
  setupVAO();
  loadLineGeometryToGPU();
  loadCartGeometryToGPU();

  loadModelViewMatrix();
  reloadProjectionMatrix();
  buildFrenet(path[0], 0.1f);
  M = TranslateMatrix(path[0] + frenetB);
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

int main(int argc, char **argv) {
  GLFWwindow *window;

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window =
      glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "The Roller Coaster Assignment", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetWindowSizeCallback(window, windowSetSizeFunc);
  glfwSetFramebufferSizeCallback(window, windowSetFramebufferSizeFunc);
  glfwSetKeyCallback(window, windowKeyFunc);
  glfwSetCursorPosCallback(window, windowMouseMotionFunc);
  glfwSetMouseButtonCallback(window, windowMouseButtonFunc);

  glfwGetFramebufferSize(window, &WIN_WIDTH, &WIN_HEIGHT);

  // Initialize glad
  if (!gladLoadGL()) {
    std::cerr << "Failed to initialise GLAD" << std::endl;
    return -1;
  }

  std::cout << "GL Version: :" << glGetString(GL_VERSION) << std::endl;
  std::cout << GL_ERROR() << std::endl;

  init(); // our own initialize stuff func

  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         !glfwWindowShouldClose(window)) {

    if (g_play) {
      animateCart();
    }

    render();
    moveCamera();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // clean up after loop
  deleteIDs();

  return 0;
}

//==================== CALLBACK FUNCTIONS ====================//

void windowSetSizeFunc(GLFWwindow *window, int width, int height) {
  WIN_WIDTH = width;
  WIN_HEIGHT = height;

  reloadProjectionMatrix();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height) {
  FB_WIDTH = width;
  FB_HEIGHT = height;

  glViewport(0, 0, FB_WIDTH, FB_HEIGHT);
}

void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      g_cursorLocked = GL_TRUE;
    } else {
      g_cursorLocked = GL_FALSE;
    }
  }
}

void windowMouseMotionFunc(GLFWwindow *window, double x, double y) {
  if (g_cursorLocked) {
    float deltaX = (x - g_cursorX) * 0.01;
    float deltaY = (y - g_cursorY) * 0.01;
    camera.rotateAroundFocus(deltaX, deltaY);

    reloadViewMatrix();
    setupModelViewProjectionTransform();
    reloadMVPUniform();
  }

  g_cursorX = x;
  g_cursorY = y;
}

void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods) {
  bool set = action != GLFW_RELEASE && GLFW_REPEAT;
  switch (key) {
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, GL_TRUE);
    break;
  case GLFW_KEY_W:
    g_moveBackForward = set ? 1 : 0;
    break;
  case GLFW_KEY_S:
    g_moveBackForward = set ? -1 : 0;
    break;
  case GLFW_KEY_A:
    g_moveLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_D:
    g_moveLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_Q:
    g_moveUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_E:
    g_moveUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_UP:
    g_rotateUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_DOWN:
    g_rotateUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_LEFT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? -1 : 0;
    else
      g_rotateLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_RIGHT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? 1 : 0;
    else
      g_rotateLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_SPACE:
    g_play = set ? !g_play : g_play;
    break;
  case GLFW_KEY_LEFT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 0.5;
    } else {
      g_panningSpeed *= 0.5;
    }
    break;
  case GLFW_KEY_RIGHT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 1.5;
    } else {
      g_panningSpeed *= 1.5;
    }
    break;
  default:
    break;
  }
  if (key == GLFW_KEY_F && (action == GLFW_PRESS))
    finalLoop = !finalLoop;
  if (key == GLFW_KEY_C && (action == GLFW_PRESS))
    firstPerson = !firstPerson;
}

//==================== OPENGL HELPER FUNCTIONS ====================//

void moveCamera() {
	if (!firstPerson){
	  Vec3f dir;

	  if (g_moveBackForward) {
	    dir += Vec3f(0, 0, g_moveBackForward * g_panningSpeed);
	  }
	  if (g_moveLeftRight) {
	    dir += Vec3f(g_moveLeftRight * g_panningSpeed, 0, 0);
	  }
	  if (g_moveUpDown) {
	    dir += Vec3f(0, g_moveUpDown * g_panningSpeed, 0);
	  }

	  if (g_rotateUpDown) {
	    camera.rotateUpDown(g_rotateUpDown * g_rotationSpeed);
	  }
	  if (g_rotateLeftRight) {
	    camera.rotateLeftRight(g_rotateLeftRight * g_rotationSpeed);
	  }
	  if (g_rotateRoll) {
	    camera.rotateRoll(g_rotateRoll * g_rotationSpeed);
	  }

	  if (g_moveUpDown || g_moveLeftRight || g_moveBackForward ||
	      g_rotateLeftRight || g_rotateUpDown || g_rotateRoll) {
	    camera.move(dir);
	  }
	} else {
		camera.m_pos = actualPos + Vec3f(0.f, 10.f, 0.f);
		camera.m_up = Vec3f(0.f, 1.f, 0.f);
		camera.m_forward = frenetT - Vec3f(0.f, 0.5f, 0.f);
	}
  reloadViewMatrix();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

std::string GL_ERROR() {
  GLenum code = glGetError();

  switch (code) {
  case GL_NO_ERROR:
    return "GL_NO_ERROR";
  case GL_INVALID_ENUM:
    return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE:
    return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION:
    return "GL_INVALID_OPERATION";
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return "GL_INVALID_FRAMEBUFFER_OPERATION";
  case GL_OUT_OF_MEMORY:
    return "GL_OUT_OF_MEMORY";
  default:
    return "Non Valid Error Code";
  }
}
