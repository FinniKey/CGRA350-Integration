
#pragma once

// glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// project
#include "opengl.hpp"
#include "cgra/cgra_mesh.hpp"


using namespace glm;
using namespace std;

struct basic_model {
	GLuint shader = 0;
	cgra::gl_mesh mesh;
	glm::vec3 color{0.7};

	//GLint diffuse = -1;
	//GLint normal = -1;
	//GLint height = -1;
	//GLint specular = -1;

	//shader parameters
	float scale = 5;
	float heightScale = 0.00;
	float tilingScale = 1;
	float POMmaxLayers = 32;

	void draw(const glm::mat4& view, const glm::mat4 proj, const glm::vec3& position, const float rotationAngle, const glm::vec3& rotationAxis, GLint diff, GLint normal, GLint height);
};

//struct sky_model {
//	GLuint shader = 0;
//	cgra::gl_mesh mesh;
//
//	GLint diffuse = -1;
//
//	void draw(const glm::mat4& view, const glm::mat4 proj);
//};

// Main application class
//
class Application {
private:
	// window
	glm::vec2 m_windowsize;
	GLFWwindow *m_window;

	// oribital camera
	float m_pitch = .86;
	float m_yaw = -.86;
	float m_distance = 2;

	// last input
	bool m_leftMouseDown = false;
	glm::vec2 m_mousePosition;

	// drawing flags
	bool m_show_axis = false;
	bool m_show_grid = false;
	bool m_showWireframe = false;

	// geometry
	basic_model m_groundPlane;
	//sky_model m_wall1;
	//sky_model m_wall2;
	//sky_model m_wall3;
	//sky_model m_wall4;


public:
	// setup
	Application(GLFWwindow *);

	// disable copy constructors (for safety)
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	// rendering callbacks (every frame)
	void render();
	void renderGUI();

	// input callbacks
	void cursorPosCallback(double xpos, double ypos);
	void mouseButtonCallback(int button, int action, int mods);
	void scrollCallback(double xoffset, double yoffset);
	void keyCallback(int key, int scancode, int action, int mods);
	void charCallback(unsigned int c);
};