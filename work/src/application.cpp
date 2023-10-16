
// std
#include <iostream>
#include <string>
#include <chrono>

// glm
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

// project
#include "application.hpp"
#include "cgra/cgra_geometry.hpp"
#include "cgra/cgra_gui.hpp"
#include "cgra/cgra_image.hpp"
#include "cgra/cgra_shader.hpp"
#include "cgra/cgra_wavefront.hpp"
#include <imgui/imgui_internal.h>

#include "geometry.h"


using namespace std;
using namespace cgra;
using namespace glm;


void basic_model::draw(const glm::mat4 &view, const glm::mat4 proj) {
	mat4 modelview = view * modelTransform;

	glUseProgram(shader); // load shader and variables
	glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, false, value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(shader, "uModelViewMatrix"), 1, false, value_ptr(modelview));
	glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, value_ptr(color));

	vec3 lightPosition = vec3(5.0f, 10.0f, 10.0f);
	glUniform3fv(glGetUniformLocation(shader, "lightPos"), 1, value_ptr(lightPosition));

	vec3 cameraPos = inverse(view)[3];
	glUniform3fv(glGetUniformLocation(shader, "viewPosition"), 1, value_ptr(cameraPos));

	glUniform1f(glGetUniformLocation(shader, "heightScale"), heightScale);
	glUniform1f(glGetUniformLocation(shader, "tilingScale"), tilingScale);
	glUniform1f(glGetUniformLocation(shader, "POMmaxLayers"), POMmaxLayers);

	glUniform1i(glGetUniformLocation(shader, "uTexture"), 0);
	glUniform1i(glGetUniformLocation(shader, "uNormal"), 1);
	glUniform1i(glGetUniformLocation(shader, "uHeight"), 2);



	glUniform1i(glGetUniformLocation(shader, "uSearchRegion"), searchRegion);
	glUniform1f(glGetUniformLocation(shader, "uDepthMode"), 1);

	// setup the depth map
	static unsigned int depthMapFBO;

	static bool depthGen = false;
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	vec2 mapSize = vec2((float)SHADOW_WIDTH, (float)SHADOW_HEIGHT);
	glUniform2fv(glGetUniformLocation(shader, "mapSize"), 1, value_ptr(mapSize));
	static unsigned int depthMap;


	if (!depthGen) {
		glGenFramebuffers(1, &depthMapFBO);
		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		depthGen = true;
	}
	//cout << depthMapFBO << endl;



	// render to the depth map
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	// configure depth stuff
	float near_plane = near, far_plane = far;
	float d = depth;
	glm::mat4 depthProjectionMatrix = glm::ortho(-d, d, -d, d, near_plane, far_plane);

	glm::mat4 depthViewMatrix = glm::lookAt(lightPosition,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix;

	// Send our transformation to the currently bound shader,
	// in the "MVP" uniform
	glUniformMatrix4fv(glGetUniformLocation(shader, "depthMVP"), 1, GL_FALSE, value_ptr(depthMVP));

	mesh.draw();

	

	// reset everything so it renders normally
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	mat4 proj2 = perspective(1.f, windowWidth / windowHeight, 0.1f, 1000.f);
	glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, false, value_ptr(proj2));

	glViewport(0, 0, windowWidth, windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform1i(glGetUniformLocation(shader, "depthMap"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glUniform1f(glGetUniformLocation(shader, "uDepthMode"), 0);
	if (depthMode) glUniform1f(glGetUniformLocation(shader, "uDepthMode"), 1);

	mesh.draw();
	

	
}


Application::Application(GLFWwindow *window) : m_window(window) {
	
	shader_builder sb;
    sb.set_shader(GL_VERTEX_SHADER, CGRA_SRCDIR + std::string("//res//shaders//color_vert.glsl"));
	sb.set_shader(GL_FRAGMENT_SHADER, CGRA_SRCDIR + std::string("//res//shaders//color_frag.glsl"));
	GLuint shader = sb.build();

	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//rocky_trail_diff_4k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE0);
	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//rocky_trail_nor_gl_4k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE1);
	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//rocky_trail_disp_4k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE2);
	m_groundPlane.shader = shader;
	m_groundPlane.mesh = geometry::plane(m_groundPlane.scale, vec3(0, 0, 0));
}




void Application::render() {
	
	// retrieve the window height
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height); 

	m_groundPlane.windowHeight = height;
	m_groundPlane.windowWidth = width;

	m_windowsize = vec2(width, height); // update window size
	glViewport(0, 0, width, height); // set the viewport to draw to the entire window

	// clear the back-buffer
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	// enable flags for normal/forward rendering
	glEnable(GL_DEPTH_TEST); 
	glDepthFunc(GL_LESS);

	// projection matrix
	mat4 proj = perspective(1.f, float(width) / height, 0.1f, 1000.f);

	// view matrix
	mat4 view = translate(mat4(1), vec3(0, 0, -m_distance))
		* rotate(mat4(1), m_pitch, vec3(1, 0, 0))
		* rotate(mat4(1), m_yaw,   vec3(0, 1, 0));

	// helpful draw options
	if (m_show_grid) drawGrid(view, proj);
	if (m_show_axis) drawAxis(view, proj);
	glPolygonMode(GL_FRONT_AND_BACK, (m_showWireframe) ? GL_LINE : GL_FILL);

	// draw the model
	m_groundPlane.draw(view, proj);
}


void Application::renderGUI() {

	// setup window
	ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiSetCond_Once);
	ImGui::Begin("Options", 0);

	// display current camera parameters
	ImGui::Text("Application %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::SliderFloat("Pitch", &m_pitch, -pi<float>() / 2, pi<float>() / 2, "%.2f");
	ImGui::SliderFloat("Yaw", &m_yaw, -pi<float>(), pi<float>(), "%.2f");
	ImGui::SliderFloat("Distance", &m_distance, 0, 100, "%.2f", 2.0f);


	ImGui::SliderFloat("POM Height", &m_groundPlane.heightScale, 0.0f, 0.25f, "%.4f");
	ImGui::SliderFloat("POM Tiling Scale", &m_groundPlane.tilingScale, 0, 10, "%.2f", 2.0f);
	ImGui::SliderFloat("POM Max Layers", &m_groundPlane.POMmaxLayers, 0, 64, "%.2f", 2.0f);

	// helpful drawing options
	ImGui::Checkbox("Show axis", &m_show_axis);
	ImGui::SameLine();
	ImGui::Checkbox("Show grid", &m_show_grid);
	ImGui::Checkbox("Wireframe", &m_showWireframe);
	ImGui::SameLine();
	if (ImGui::Button("Screenshot")) rgba_image::screenshot(true);

	
	ImGui::Separator();

	// example of how to use input boxes
	static float exampleInput;
	if (ImGui::InputFloat("example input", &exampleInput)) {
		cout << "example input changed to " << exampleInput << endl;
	}

	ImGui::Separator();

	const char* items[] = { "Normal", "Light perspective" };
	if (ImGui::Combo("Mode", &mode, items, IM_ARRAYSIZE(items))) {
		m_groundPlane.depthMode = false;
		if (mode == 1) m_groundPlane.depthMode = 1;
	}

	ImGui::SliderFloat("Near", &m_groundPlane.near, 1, 10);
	ImGui::SliderFloat("Far", &m_groundPlane.far, 1, 50);
	ImGui::SliderFloat("Depth", &m_groundPlane.depth, 1, 100);

	// finish creating window
	ImGui::End();
}


void Application::cursorPosCallback(double xpos, double ypos) {
	if (m_leftMouseDown) {
		vec2 whsize = m_windowsize / 2.0f;

		// clamp the pitch to [-pi/2, pi/2]
		m_pitch += float(acos(glm::clamp((m_mousePosition.y - whsize.y) / whsize.y, -1.0f, 1.0f))
			- acos(glm::clamp((float(ypos) - whsize.y) / whsize.y, -1.0f, 1.0f)));
		m_pitch = float(glm::clamp(m_pitch, -pi<float>() / 2, pi<float>() / 2));

		// wrap the yaw to [-pi, pi]
		m_yaw += float(acos(glm::clamp((m_mousePosition.x - whsize.x) / whsize.x, -1.0f, 1.0f))
			- acos(glm::clamp((float(xpos) - whsize.x) / whsize.x, -1.0f, 1.0f)));
		if (m_yaw > pi<float>()) m_yaw -= float(2 * pi<float>());
		else if (m_yaw < -pi<float>()) m_yaw += float(2 * pi<float>());
	}

	// updated mouse position
	m_mousePosition = vec2(xpos, ypos);
}


void Application::mouseButtonCallback(int button, int action, int mods) {
	(void)mods; // currently un-used

	// capture is left-mouse down
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		m_leftMouseDown = (action == GLFW_PRESS); // only other option is GLFW_RELEASE
}


void Application::scrollCallback(double xoffset, double yoffset) {
	(void)xoffset; // currently un-used
	m_distance *= pow(1.1f, -yoffset);
}


void Application::keyCallback(int key, int scancode, int action, int mods) {
	(void)key, (void)scancode, (void)action, (void)mods; // currently un-used
}


void Application::charCallback(unsigned int c) {
	(void)c; // currently un-used
}
