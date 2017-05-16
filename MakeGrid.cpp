#include <iostream>
// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>
// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// Math
#define _USE_MATH_DEFINES
#include <math.h>
// Time
#include <ctime>
// Shader class
#include "Shader.h"
// SimplexNoise
#include "SimplexNoise.h"

// Function prototypes
void makeBuffers(bool stop, float zY);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Window pixel dimensions
GLFWwindow* window;
const GLuint WIDTH = 600, HEIGHT = 600;
// total width and height of coordinate plane
const GLfloat coordW = 2.0f, coordH = 2.0f;
// dimensions of mesh in tiles per row or column
const GLuint dimensions = 30;
// width and height of each tile in opengl coordinate plane
const GLfloat squareW = coordW / dimensions, squareH = coordH / dimensions;

// Noise generator
SimplexNoise noiseGenerator;

// Buffers declared here because they are redrawn every frame
GLuint VBO, VAO, VBOoutline, VAOoutline, EBO;
// Counters for vertices and indices
int numVertices;
int numIndices;
// Drawing in seperate triangle strips to avoid ghost connecting lines
const int numStripsRequired = dimensions;
// Degenerate triangles to connect strips
const int numDegensRequired = 2 * (numStripsRequired - 1);
const int verticesPerStrip = 2 * (dimensions + 1);
GLuint indices[((verticesPerStrip * numStripsRequired) + numDegensRequired) * sizeof(GLuint)];
// Vertices
GLfloat vertices[((dimensions + 1) * (dimensions + 1) * 6) * sizeof(GLfloat)];
// Outline vertices
GLfloat verticesOutline[((dimensions + 1) * (dimensions + 1) * 6) * sizeof(GLfloat)];
// "Flight speed"
float zY = 0.0f;
// Color holders
float r, g, b;
GLfloat colors[5000] = {};

// The MAIN function, from here we start the application and run the game loop
int main()
{
	// random seed
	srand(time(NULL));

	std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	window = glfwCreateWindow(WIDTH, HEIGHT, "Mountains", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	// Define the viewport dimensions
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Build and compile shader program *not going to work without changing path*
	Shader ourShader("C:/Users/dja94/Documents/Visual Studio 2015/Projects/CreatingAWindow/CreatingAWindow/shader.vs",
		"C:/Users/dja94/Documents/Visual Studio 2015/Projects/CreatingAWindow/CreatingAWindow/shader.frag");

	float r = rand() / (float)RAND_MAX;
	float g = rand() / (float)RAND_MAX;
	float b = rand() / (float)RAND_MAX;
	makeBuffers(false, zY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glLineWidth(2.0f);
	
	float angle = 0.0f;
	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Clear the colorbuffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::vec2 SCREEN_SIZE(WIDTH, HEIGHT);	// Screen size
		glm::mat4 projectionMatrix;				// Store the projection matrix
		glm::mat4 viewMatrix;					// Store the view matrix
		glm::mat4 modelMatrix;					// Store the model matrix

		// Set PVM matrices and do rotation for flight view
		projectionMatrix = glm::perspective(45.0f, (float)SCREEN_SIZE.x / (float)SCREEN_SIZE.y, 0.1f, 100.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -1.0f));
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::rotate(modelMatrix, glm::radians(-75.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		// Send to shader
		glm::mat4 MVP = projectionMatrix * viewMatrix * modelMatrix;
		GLuint mvpLoc = glGetUniformLocation(ourShader.Program, "MVP");
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

		// Calculate offset and start shader
		int offset = (numIndices / dimensions) -1;
		ourShader.Use();

		// Draw outline of mesh first
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
		glBindVertexArray(VAOoutline);
		// Draw multiple triangle strips to get rid of degenerate lines
		for (int strip2 = 0; strip2 < dimensions; strip2++) {
			glDrawElements(GL_TRIANGLE_STRIP, offset, GL_UNSIGNED_INT, (GLvoid*)((strip2*(offset))*sizeof(GLuint)));
		}
		glDisable(GL_POLYGON_OFFSET_FILL);
		glBindVertexArray(0);
		// Draw fill second to hide connecting lines
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(VAO);
		// Draw multiple triangle strips to get rid of degenerate lines
		for (int strip = 0; strip < dimensions; strip++) {
			glDrawElements(GL_TRIANGLE_STRIP, offset, GL_UNSIGNED_INT, (GLvoid*)((strip*(offset)) * sizeof(GLuint)));
		}
		glBindVertexArray(0);

		// Simulate flying by changing y starting point
		zY += .015;
		makeBuffers(true, zY);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Generate vertex and index data
// Fill buffers and send to GPU
void makeBuffers(bool stop, float zY) {
	// ------------ Generate vertices ------------
	int i = 0, i2 = 0, c = 0;
	float zX = 0.0f;
	for (GLfloat y = 0.0f; y < coordH+squareH; y += squareH) {
		for (GLfloat x = 0.0f; x < coordW+squareW; x += squareW) {
			// Z value calculated using simplex noise
			GLfloat simplex = noiseGenerator.noise(zX, zY);
			vertices[i++] = x - coordW/2;					
			vertices[i++] = y - coordH/2;					
			vertices[i++] = simplex;
	
			// Random mountain colors
			// fill array on first frame for constant values after initial rand
			if (stop == 0) {
				r = rand() / (float)RAND_MAX;
				g = rand() / (float)RAND_MAX;
				b = rand() / (float)RAND_MAX;
				colors[c++] = r;
				colors[c++] = g;
				colors[c++] = b;

				vertices[i++] = r;	// r
				vertices[i++] = g;	// g
				vertices[i++] = b;	// b
				vertices[i++] = 1.0f;
			}else{
				vertices[i++] = colors[c++];	// r
				vertices[i++] = colors[c++];	// g
				vertices[i++] = colors[c++];	// b
				vertices[i++] = 1.0f;
			}

			// ------------ Outline draw ----------------
			verticesOutline[i2++] = x - coordW / 2;						
			verticesOutline[i2++] = y - coordH / 2;						
			verticesOutline[i2++] = simplex;

			verticesOutline[i2++] = 0.0f;
			verticesOutline[i2++] = 0.0f;
			verticesOutline[i2++] = 0.0f;
			verticesOutline[i2++] = 0.75f;

			numVertices++;
			zX = zX + .15f;
		}
		zX = 0.0f;
		zY = zY + .15f;
	c = 0;
	// ------------ Generate indices ------------
	}
	int index = 0;
	GLuint start = 0;
	GLuint dAdd = 1;
	for (int row = 0; row < dimensions; row++) {
		for (int col = 0; col < dimensions+1; col++) {
			indices[index++] = start;
			indices[index++] = dimensions + dAdd;

			start++;
			dAdd++;
		}
	}
	numIndices = (verticesPerStrip * numStripsRequired) + numDegensRequired;

	// Make buffers
	glGenVertexArrays(1, &VAO);
	glGenVertexArrays(1, &VAOoutline);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &VBOoutline);
	glGenBuffers(1, &EBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

	 // 2nd draw - Outline
	glBindVertexArray(VAOoutline);

	glBindBuffer(GL_ARRAY_BUFFER, VBOoutline);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesOutline), verticesOutline, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 

	glBindVertexArray(0);
}

// Called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}