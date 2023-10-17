
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

#include "geometry.h"
#include <imgui_internal.h>
#include <glm/gtc/random.hpp>


using namespace std;
using namespace cgra;
using namespace glm;


void basic_model::draw(const glm::mat4& view, const glm::mat4 proj, const glm::vec3& position, const float rotationAngle, const glm::vec3& rotationAxis, GLint diff, GLint normal, GLint height) {

	//glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, false, value_ptr(proj));
	//glUniformMatrix4fv(glGetUniformLocation(shader, "uModelViewMatrix"), 1, false, value_ptr(modelview));
	glUseProgram(shader); // load shader and variables

	vec3 lightPosition = vec3(1.0f, 2.0f, 2.0f);
	glUniform3fv(glGetUniformLocation(shader, "lightPos"), 1, value_ptr(lightPosition));

	vec3 cameraPos = inverse(view)[3];
	glUniform3fv(glGetUniformLocation(shader, "viewPosition"), 1, value_ptr(cameraPos));

	glUniform1f(glGetUniformLocation(shader, "heightScale"), heightScale);
	glUniform1f(glGetUniformLocation(shader, "tilingScale"), tilingScale);
	glUniform1f(glGetUniformLocation(shader, "POMmaxLayers"), POMmaxLayers);

	glUniform1i(glGetUniformLocation(shader, "uTexture"), diff);
	glUniform1i(glGetUniformLocation(shader, "uNormal"), normal);
	glUniform1i(glGetUniformLocation(shader, "uHeight"), height);

	// ------------------- start of depth map stuff

	glUniform1i(glGetUniformLocation(shader, "uSearchRegion"), searchRegion);
	glUniform1f(glGetUniformLocation(shader, "uDepthMode"), 1);

	//// setup the depth map
	static unsigned int depthMapFBO;

	static bool depthGen = false;
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	vec2 mapSize = vec2((float)SHADOW_WIDTH, (float)SHADOW_HEIGHT);
	glUniform2fv(glGetUniformLocation(shader, "mapSize"), 1, value_ptr(mapSize));
	static unsigned int depthMap;

	if (!depthGen) {
		glGenFramebuffers(1, &depthMapFBO); // Generate only one framebuffer
		glGenTextures(1, &depthMap);        // Generate only one texture

		// Configure depthMap texture
		glActiveTexture(GL_TEXTURE7); // Use texture unit 6
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Bind depthMap to the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		depthGen = true;
	}
	//cout << depthMapFBO << endl;

	//// render to the depth map
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	//cout << near << endl;
	float near_plane = mapnear, far_plane = mapfar;
	float d = mapdepth;
	glm::mat4 depthProjectionMatrix = glm::ortho(-d, d, -d, d, near_plane, far_plane);

	glm::mat4 depthViewMatrix = glm::lookAt(lightPosition,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix;

	glUniformMatrix4fv(glGetUniformLocation(shader, "depthMVP"), 1, GL_FALSE, value_ptr(depthMVP));

	//mesh.draw();

	//reset everything so it renders normally
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ------------- end of depth map stuff

	mat4 modelTransform = mat4(1.0f);
	modelTransform = glm::translate(modelTransform, position);
	modelTransform = glm::rotate(modelTransform, glm::radians(rotationAngle), rotationAxis);

	mat4 modelview = view * modelTransform;

	mat4 proj2 = perspective(1.f, (float)wWidth / (float)wHeight, 0.1f, 1000.f);
	
	glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, false, value_ptr(proj2));
	glUniformMatrix4fv(glGetUniformLocation(shader, "uModelViewMatrix"), 1, false, value_ptr(modelview));
	glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, value_ptr(color));
	
	glViewport(0, 0, wWidth, wHeight);

	glUniform1i(glGetUniformLocation(shader, "depthMap"), 7);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, 7);

	glUniform1f(glGetUniformLocation(shader, "uDepthMode"), 0);
	if (depthMode) glUniform1f(glGetUniformLocation(shader, "uDepthMode"), 1);

	mesh.draw(); // draw    // this gl_mesh method is defined in cgra_mesh.cpp
}


Application::Application(GLFWwindow *window) : m_window(window) {
	srand(time(0));
	
	shader_builder sb;
    sb.set_shader(GL_VERTEX_SHADER, CGRA_SRCDIR + std::string("//res//shaders//color_vert.glsl"));
	sb.set_shader(GL_FRAGMENT_SHADER, CGRA_SRCDIR + std::string("//res//shaders//color_frag.glsl"));
	jamie_shader = sb.build();

	sb.set_shader(GL_VERTEX_SHADER, CGRA_SRCDIR + std::string("//res//shaders//color_vert_project.glsl"));
	sb.set_shader(GL_FRAGMENT_SHADER, CGRA_SRCDIR + std::string("//res//shaders//color_frag_project.glsl"));
	just_shader = sb.build();

	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//rocky_trail_diff_4k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE1);
	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//rocky_trail_nor_gl_4k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE2);
	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//rocky_trail_disp_4k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE3);


	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//castle_brick_02_red_diff_2k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE4);
	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//castle_brick_02_red_nor_gl_2k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE5);
	cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//castle_brick_02_red_disp_2k.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE6);

	//cgra::rgba_image(CGRA_SRCDIR + string("//res//textures//back.jpg")).uploadTexture(GL_RGB8, GL_TEXTURE6);

	spawnBoids(boidNum);
}

void Application::spawnBoids(int numBoids) {
	boids.clear();
	cout << "spawn boid" << endl;
	for (int i = 0; i < numBoids; i++) {
		boid b;
		b.id = i;
		b.model.mesh = geometry::plane(0.1);
		b.pos = linearRand(vec3(-1), vec3(1));
		b.vel = vec3(0.001);
		b.acc = vec3(0);
		boids.push_back(b);
	}
}

vec3 clampVector(vec3 v, float minVal, float maxVal) {
	vec3 r = v;

	if (v.x < minVal) r.x = minVal;
	else if (v.x > maxVal) r.x = maxVal;

	if (v.y < minVal) r.y = minVal;
	else if (v.y > maxVal) r.y = maxVal;
	
	if (v.z < minVal) r.z = minVal;
	else if (v.z > maxVal) r.z = maxVal;

	return r;
}

void Application::render() {
	
	// retrieve the window height
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height); 
	wWidth = width;
	wHeight = height;

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

	mat4 modelView = mat4(1.0f);

	// draw the model

	// Jamies planes

	m_groundPlane.shader = jamie_shader;
	m_groundPlane.mesh = geometry::plane(10);

	m_groundPlane.draw(view, proj, glm::vec3(0.0f, -2.5f, 5.0f), -90.0f, glm::vec3(1, 0, 0), 4, 5, 6);
	m_groundPlane.draw(view, proj, glm::vec3(0.0f, -2.5f, -5.0f), 90.0f, glm::vec3(1, 0, 0), 4, 5, 6);
	m_groundPlane.draw(view, proj, glm::vec3(-5.0f, -2.5f, 0.0f), -90.0f, glm::vec3(0, 0, 1), 4, 5, 6);
	m_groundPlane.draw(view, proj, glm::vec3(5.0f, -2.5f, 0.0f), 90.0f, glm::vec3(0, 0, 1), 4, 5, 6);
	m_groundPlane.draw(view, proj, glm::vec3(0.0f, -.25f, 0.0f), 0.0f, glm::vec3(1, 0, 0), 1, 2, 3);


	// Aidans particle system

	p_system.draw(view, proj);

	
	//-------- ryan's render start -----------------------------------------

	// draw all objects
	for (int i = 0; i < m_all_objects.size(); i++) {
		if (m_all_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
			m_all_objects[i]->draw(view, proj, vec3(0.0, 0.0, 0.0), 0.0f, vec3(1, 0, 0), 0, 1, 2);
			//cout << "m_all_objects[i]->shader: " << m_all_objects[i]->shader << endl;
		}
		else {
			cerr << "null pointer" << endl;
		}
	}



	// draw log pile objects
	for (int i = 0; i < m_log_pile_objects.size(); i++) {
		if (m_log_pile_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
			m_log_pile_objects[i]->draw(view, proj, vec3(0.0, 0.0, 0.0), 0.0f, vec3(1, 0, 0), 0, 1, 2);
			//cout << "m_all_objects[i]->shader: " << m_all_objects[i]->shader << endl;
		}
		else {
			cerr << "null pointer" << endl;
		}
	}



	// draw fire guard objects
	for (int i = 0; i < m_fire_guard_objects.size(); i++) {
		if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
			m_fire_guard_objects[i]->draw(view, proj, vec3(0.0, 0.0, 0.0), 0.0f, vec3(1, 0, 0), 0, 1, 2);
			//cout << "m_all_objects[i]->shader: " << m_all_objects[i]->shader << endl;
		}
		else {
			cerr << "null pointer" << endl;
		}
	}



	// draw tree objects
	for (int i = 0; i < m_tree_objects.size(); i++) {
		if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
			m_tree_objects[i]->draw(view, proj, vec3(0.0, 0.0, 0.0), 0.0f, vec3(1, 0, 0), 0, 1, 2);
			//cout << "m_all_objects[i]->shader: " << m_all_objects[i]->shader << endl;
		}
		else {
			cerr << "null pointer" << endl;
		}
	}


	// draw chair objects
	for (int i = 0; i < m_chair_objects.size(); i++) {
		if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
			m_chair_objects[i]->draw(view, proj, vec3(0.0, 0.0, 0.0), 0.0f, vec3(1, 0, 0), 0, 1, 2);
			//cout << "m_all_objects[i]->shader: " << m_all_objects[i]->shader << endl;
		}
		else {
			cerr << "null pointer" << endl;
		}
	}

	// draw table objects
	for (int i = 0; i < m_table_objects.size(); i++) {
		if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
			m_table_objects[i]->draw(view, proj, vec3(0.0, 0.0, 0.0), 0.0f, vec3(1, 0, 0), 0, 1, 2);
			//cout << "m_all_objects[i]->shader: " << m_all_objects[i]->shader << endl;
		}
		else {
			cerr << "null pointer" << endl;
		}
	}


	// draw window objects
	for (int i = 0; i < m_window_objects.size(); i++) {
		if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
			m_window_objects[i]->draw(view, proj, vec3(0.0, 0.0, 0.0), 0.0f, vec3(1, 0, 0), 0, 1, 2);
			//cout << "m_all_objects[i]->shader: " << m_all_objects[i]->shader << endl;
		}
		else {
			cerr << "null pointer" << endl;
		}
	}


	// --------- ryan's render end --------------------------------------

	for (boid &b : boids) {
		
		/*cout << "pos " << b.pos.x << " " << b.pos.y << " " << b.pos.z << endl;
		cout << "vel " << b.vel.x << " " << b.vel.y << " " << b.vel.z << endl;
		cout << "acc " << b.acc.x << " " << b.acc.y << " " << b.acc.z << endl;*/

		

		b.model.shader = jamie_shader;

		b.acc = glm::vec3(0);

		glm::vec3 avoidance = glm::vec3(0);
		glm::vec3 cohesion = glm::vec3(0);
		glm::vec3 alignment = glm::vec3(0);

		vector<boid> neighs;
		// get neighs
		float sightRadius = 0.5;
		for (boid o : boids) {
			float d = distance(b.pos, o.pos);
			if (b.id != o.id && d <= sightRadius) {
				neighs.push_back(o);
			}
		}
		
		//cout << "neigh num " << neighs.size() << endl;

		// avoidance
		for (boid o : neighs) {
			float weight = 1.0;
			glm::vec3 displacement = b.pos - o.pos;
			float distance = length(displacement);
			avoidance += (displacement / (distance * distance));
		}

		if (neighs.size() > 0) {

			//cohesion
			glm::vec3 avgBPos = glm::vec3(0);

			for (boid curB : neighs) {
				avgBPos += curB.pos;
			}
			avgBPos /= neighs.size();
			cohesion = avgBPos - b.pos;

			// alignment
			vec3 avgBVel = vec3(0);
			for (boid curB : neighs) {
				avgBVel += curB.vel;
			}
			avgBVel /= neighs.size();
			alignment = avgBVel - b.vel;

			//cout << "alignment " << alignment.x << alignment.y << alignment.z << endl;

			
		}
		b.acc = (avoidance + cohesion + alignment);
		

		

		// clamp and adjust positions
		float minSpeed = 0.0005;
		float maxSpeed = 0.05;

		b.vel += b.acc;

		float speed = length(b.vel);

		if (speed > maxSpeed) speed = maxSpeed;
		if (speed < minSpeed) speed = minSpeed;
		//if (speed < scene->minSpeed() * 1.5 && m_team < 0) speed = scene->minSpeed() * 1.5;
		//if (speed > scene->maxSpeed() * 1.5 && m_team < 0) speed = scene->maxSpeed() * 1.5;

	
		b.vel = vec3(speed) * normalize(b.vel);

	
		b.pos += b.vel;
		
		// clamp position
		// change later to be based on distance
		vec3 sceneBound = vec3(3.0);
		if (b.pos.x < -sceneBound.x || b.pos.x > sceneBound.x
			|| b.pos.y < -sceneBound.y || b.pos.y > sceneBound.y
			|| b.pos.z < -sceneBound.z || b.pos.z > sceneBound.z) {

			b.vel += (vec3(0) - b.pos) / vec3(100 / 0.4);
		}

		if (b.pos.y < 0) {
			b.vel.y = -b.vel.y;
			b.pos.y = 0;
		}
		/*if (b.pos.x > sceneBound.x) b.pos.x = -sceneBound.x;
		if (b.pos.x < -sceneBound.x) b.pos.x = sceneBound.x;

		if (b.pos.y > sceneBound.y) b.pos.y = -sceneBound.y;
		if (b.pos.y < -sceneBound.y) b.pos.y = sceneBound.y;

		if (b.pos.z > sceneBound.z) b.pos.z = -sceneBound.z;
		if (b.pos.z < -sceneBound.z) b.pos.z = sceneBound.z;*/

		b.model.draw(view, proj, b.pos, 0, vec3(1, 0, 0), 0, 0, 0);
	}



}


void Application::renderGUI() {

	// setup window
	ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_Once);
	ImGui::Begin("Options", 0);

	// display current camera parameters
	ImGui::Text("Application %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::SliderFloat("Pitch", &m_pitch, -pi<float>() / 2, pi<float>() / 2, "%.2f");
	ImGui::SliderFloat("Yaw", &m_yaw, -pi<float>(), pi<float>(), "%.2f");
	ImGui::SliderFloat("Distance", &m_distance, 0, 100, "%.2f", 2.0f);


	ImGui::SliderFloat("POM Height", &m_groundPlane.heightScale, 0.0f, 0.25f, "%.4f");
	ImGui::SliderFloat("POM Tiling Scale", &m_groundPlane.tilingScale, 0, 10, "%.2f", 2.0f);
	ImGui::SliderFloat("POM Max Layers", &m_groundPlane.POMmaxLayers, 0, 256, "%.2f", 2.0f);

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

	// boids stuff starts

	if (ImGui::SliderInt("Boid num", &boidNum, 0, 10)) {
		spawnBoids(boidNum);
	}

	// boids stuff ends

	// depth stuff starts
	ImGui::Separator();

	const char* items[] = { "Default", "Light perspective" };

	if (ImGui::Combo("Mode", &mode, items, IM_ARRAYSIZE(items))) {
		depthMode = false;
		if (mode == 0) depthMode = false;
		if (mode == 1) depthMode = true;
	}

	ImGui::SliderFloat("Near", &mapnear, 1, 10);
	ImGui::SliderFloat("Far", &mapfar, 1, 50);
	ImGui::SliderFloat("Depth", &mapdepth, 10, 100);

	ImGui::Separator();
	// depth stuff ends

	// --------------- Ryan's IMGUI start-----------------------------------

	// ------- tree start ---------------------------------
	if (ImGui::Button("Tree")) {
		is_tree = true;
		tree_initial_draw = -0.01;

	}
	if (is_tree) {

		tree_initial_draw += 0.01;
		if (tree_initial_draw > -0.01 && tree_initial_draw < 0.005) {

			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}


		if ((ImGui::SliderFloat3("Tree Position", tree_position, -5.0, 5.0))) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderInt("Tree Subdivisions", &tree_subdiv, 1, 64)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderFloat("Tree Height", &tree_height, 0.001, 10.0)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderFloat("Tree Top Radius", &tree_top_radius, 0.001, 2)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderFloat("Tree Bottom Radius", &tree_bottom_radius, 0.001, 5)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderInt("Branch Subdivisions", &branch_subdiv, 1, 32)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderInt("Number Branch Rows", &num_branch_rows, 1, 50)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}


		if (ImGui::SliderFloat("Branches Start", &branches_start, 0.001, tree_height + tree_height * 0.2)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}


		if (ImGui::SliderFloat("Branches End", &branches_end, 0.001, tree_height - 0.001)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderFloat("Branch Top Radius", &branch_top_radius, 0.001, 3)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}

		if (ImGui::SliderFloat("Branch Bottom Radius", &branch_bottom_radius, 0.001, 8)) {
			for (int i = 0; i < m_tree_objects.size(); i++) {
				if (m_tree_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_tree_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_tree_objects.clear();
			tree(tree_subdiv, tree_height, tree_top_radius, tree_bottom_radius,   // tree parameters
				branch_subdiv, num_branch_rows, branches_start, branches_end, branch_top_radius, branch_bottom_radius,  // branch parameters
				uniform_scale, tree_position[0], tree_position[1], tree_position[2], tree_rotation[0], tree_rotation[1], tree_rotation[2]);
		}


	}

	// ------- tree end ---------------------------------

		// ----- table start -----------------------------
	if (ImGui::Button("Table")) {
		is_table = true;
		table_initial_draw = -0.01;

	}
	if (is_table) {


		table_initial_draw += 0.01;
		if (table_initial_draw > -0.01 && table_initial_draw < 0.005) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_x_position, table_y_position, table_z_position,
				table_x_rotation, table_y_rotation, table_z_rotation,
				table_x_rotation_2, table_y_rotation_2, table_z_rotation_2,
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}

		if ((ImGui::SliderFloat3("Table Position", table_position, -5.0, 5.0))) {
			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_position[0], table_position[1], table_position[2],
				table_rotation[0], table_rotation[1], table_rotation[2],
				table_rotation_2[0], table_rotation_2[1], table_rotation_2[2],
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}

		if ((ImGui::SliderFloat3("Table Rotation", table_rotation_2, -360.0, 360.0))) {

			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_position[0], table_position[1], table_position[2],
				table_rotation[0], table_rotation[1], table_rotation[2],
				table_rotation_2[0], table_rotation_2[1], table_rotation_2[2],
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}

		if (ImGui::SliderFloat("Table Width", &table_top_width, 0.001, 1.0)) {
			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_position[0], table_position[1], table_position[2],
				table_rotation[0], table_rotation[1], table_rotation[2],
				table_rotation_2[0], table_rotation_2[1], table_rotation_2[2],
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}

		if (ImGui::SliderFloat("Table Top Height", &table_top_height, 0.001, 0.05)) {

			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_position[0], table_position[1], table_position[2],
				table_rotation[0], table_rotation[1], table_rotation[2],
				table_rotation_2[0], table_rotation_2[1], table_rotation_2[2],
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}

		if (ImGui::SliderFloat("Table Depth", &table_top_depth, 0.001, 2.0)) {

			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_position[0], table_position[1], table_position[2],
				table_rotation[0], table_rotation[1], table_rotation[2],
				table_rotation_2[0], table_rotation_2[1], table_rotation_2[2],
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}


		if (ImGui::SliderFloat("Table Leg Width & Depth", &table_leg_width_depth, 0.001, 0.05)) {

			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_position[0], table_position[1], table_position[2],
				table_rotation[0], table_rotation[1], table_rotation[2],
				table_rotation_2[0], table_rotation_2[1], table_rotation_2[2],
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}

		if (ImGui::SliderFloat("Table Leg Height", &table_leg_height, 0.001, 1.0)) {

			for (int i = 0; i < m_table_objects.size(); i++) {
				if (m_table_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_table_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_table_objects.clear();

			table(table_top_width, table_top_height, table_top_depth,
				table_position[0], table_position[1], table_position[2],
				table_rotation[0], table_rotation[1], table_rotation[2],
				table_rotation_2[0], table_rotation_2[1], table_rotation_2[2],
				table_leg_width_depth, table_leg_height, table_uniform_scale);
		}



	}

	// ----- table end -----------------------------

	//  ---------  chair start ----------------------------------------
	if (ImGui::Button("Chair")) {
		is_chair = true;
		chair_initial_draw = -0.01;

	}
	if (is_chair) {


		chair_initial_draw += 0.01;
		if (chair_initial_draw > -0.01 && chair_initial_draw < 0.005) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_x_position, seat_y_position, seat_z_position,
				seat_x_rotation, seat_y_rotation, seat_z_rotation,
				seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2,
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if ((ImGui::SliderFloat3("Chair Position", seat_position, -5.0, 5.0))) {
			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if ((ImGui::SliderFloat3("Chair Rotation", seat_rotation_2, -360.0, 360.0))) {

			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if (ImGui::SliderFloat("Chair Seat Width", &seat_width, 0.001, 2.0)) {
			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if (ImGui::SliderFloat("Chair Seat Height", &seat_height, 0.001, 0.1)) {

			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if (ImGui::SliderFloat("Chair Seat Depth", &seat_depth, 0.001, 0.3)) {

			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}


		if (ImGui::SliderFloat("Chair Leg Width & Depth", &leg_width_depth, 0.001, 0.35)) {

			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if (ImGui::SliderFloat("Chair Leg Height", &leg_height, 0.001, 0.5)) {

			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if (ImGui::SliderFloat("Chair Back Support Height", &b_support_height, 0.001, 0.5)) {

			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}

		if (ImGui::SliderFloat("Chair Back Rest Height", &b_support_rest_height, 0.001, 0.5)) {

			for (int i = 0; i < m_chair_objects.size(); i++) {
				if (m_chair_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_chair_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_chair_objects.clear();

			chair(seat_width, seat_height, seat_depth,
				seat_position[0], seat_position[1], seat_position[2],
				seat_rotation[0], seat_rotation[1], seat_rotation[2],
				seat_rotation_2[0], seat_rotation_2[1], seat_rotation_2[2],
				leg_width_depth, leg_height,
				b_support_height, b_support_rest_height, chair_uniform_scale);
		}




	}

	//------------ chair end --------------------------------



	// ------ fire guard start ------------------------------
	if (ImGui::Button("Fire Gaurd")) {
		is_fire_guard = true;
		fire_guard_initial_draw = -0.01;

	}
	if (is_fire_guard) {

		fire_guard_initial_draw += 0.01;
		if (fire_guard_initial_draw > -0.01 && fire_guard_initial_draw < 0.005) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_fire_guard_objects.size(); i++) {
				if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_fire_guard_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_fire_guard_objects.clear();

			fire_guard(fire_guard_radius, fire_guard_subdiv, num_brick_rows,
				brick_width, brick_height, brick_depth,
				fire_guard_position[0], fire_guard_position[1], fire_guard_position[2],
				fire_guard_rotation[0], fire_guard_rotation[1], fire_guard_rotation[2]);
		}



		if (ImGui::SliderFloat("Fire Guard Radius", &fire_guard_radius, 0.001, 3.0)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_fire_guard_objects.size(); i++) {
				if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_fire_guard_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_fire_guard_objects.clear();

			fire_guard(fire_guard_radius, fire_guard_subdiv, num_brick_rows,
				brick_width, brick_height, brick_depth,
				fire_guard_position[0], fire_guard_position[1], fire_guard_position[2],
				fire_guard_rotation[0], fire_guard_rotation[1], fire_guard_rotation[2]);
		}

		if (ImGui::SliderInt("Number of Bricks", &fire_guard_subdiv, 1, 48)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_fire_guard_objects.size(); i++) {
				if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_fire_guard_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_fire_guard_objects.clear();

			fire_guard(fire_guard_radius, fire_guard_subdiv, num_brick_rows,
				brick_width, brick_height, brick_depth,
				fire_guard_position[0], fire_guard_position[1], fire_guard_position[2],
				fire_guard_rotation[0], fire_guard_rotation[1], fire_guard_rotation[2]);
		}

		if (ImGui::SliderInt("Number of Brick Rows", &num_brick_rows, 1, 5)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_fire_guard_objects.size(); i++) {
				if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_fire_guard_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_fire_guard_objects.clear();

			fire_guard(fire_guard_radius, fire_guard_subdiv, num_brick_rows,
				brick_width, brick_height, brick_depth,
				fire_guard_position[0], fire_guard_position[1], fire_guard_position[2],
				fire_guard_rotation[0], fire_guard_rotation[1], fire_guard_rotation[2]);
		}

		if (ImGui::SliderFloat("Brick Width", &brick_width, 0.001, 0.5)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_fire_guard_objects.size(); i++) {
				if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_fire_guard_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_fire_guard_objects.clear();

			fire_guard(fire_guard_radius, fire_guard_subdiv, num_brick_rows,
				brick_width, brick_height, brick_depth,
				fire_guard_position[0], fire_guard_position[1], fire_guard_position[2],
				fire_guard_rotation[0], fire_guard_rotation[1], fire_guard_rotation[2]);
		}

		if (ImGui::SliderFloat("Brick Height", &brick_height, 0.001, 0.5)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_fire_guard_objects.size(); i++) {
				if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_fire_guard_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_fire_guard_objects.clear();

			fire_guard(fire_guard_radius, fire_guard_subdiv, num_brick_rows,
				brick_width, brick_height, brick_depth,
				fire_guard_position[0], fire_guard_position[1], fire_guard_position[2],
				fire_guard_rotation[0], fire_guard_rotation[1], fire_guard_rotation[2]);
		}

		if (ImGui::SliderFloat("Brick Depth", &brick_depth, 0.001, 0.5)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_fire_guard_objects.size(); i++) {
				if (m_fire_guard_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_fire_guard_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_fire_guard_objects.clear();

			fire_guard(fire_guard_radius, fire_guard_subdiv, num_brick_rows,
				brick_width, brick_height, brick_depth,
				fire_guard_position[0], fire_guard_position[1], fire_guard_position[2],
				fire_guard_rotation[0], fire_guard_rotation[1], fire_guard_rotation[2]);
		}


	}

	//----------- fire guard end ---------------------------------

	//---------- log pile start ------------------------------------------------
	// log pile
	if (ImGui::Button("Log Pile")) {
		is_log_pile = true;
		log_pile_initial_draw = -0.01;

	}
	if (is_log_pile) {


		log_pile_initial_draw += 0.01;
		if (log_pile_initial_draw > -0.01 && log_pile_initial_draw < 0.005) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_log_pile_objects.size(); i++) {
				if (m_log_pile_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_log_pile_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_log_pile_objects.clear();
			log_pile(num_bottom_logs, log_subdiv, log_radius, log_length,
				log_position[0], log_position[1], log_position[2],
				log_rotation[0], log_rotation[1], log_rotation[2]);

		}

		if (ImGui::SliderInt("Number of Bottom Logs", &num_bottom_logs, 1, 8)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_log_pile_objects.size(); i++) {
				if (m_log_pile_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_log_pile_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_log_pile_objects.clear();
			log_pile(num_bottom_logs, log_subdiv, log_radius, log_length,
				log_position[0], log_position[1], log_position[2],
				log_rotation[0], log_rotation[1], log_rotation[2]);

		}

		if (ImGui::SliderInt("Log Subdivision", &log_subdiv, 1, 32)) {

			for (int i = 0; i < m_log_pile_objects.size(); i++) {
				if (m_log_pile_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_log_pile_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_log_pile_objects.clear();
			log_pile(num_bottom_logs, log_subdiv, log_radius, log_length,
				log_position[0], log_position[1], log_position[2],
				log_rotation[0], log_rotation[1], log_rotation[2]);

		}

		if ((ImGui::SliderFloat3("Log Position", log_position, -5.0, 5.0))) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_all_objects.size(); i++) {
				if (m_log_pile_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_log_pile_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_log_pile_objects.clear();
			log_pile(num_bottom_logs, log_subdiv, log_radius, log_length,
				log_position[0], log_position[1], log_position[2],
				log_rotation[0], log_rotation[1], log_rotation[2]);

		}


		if (ImGui::SliderFloat("Log Radius", &log_radius, 0.001, 0.1)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_log_pile_objects.size(); i++) {
				if (m_log_pile_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_log_pile_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_log_pile_objects.clear();
			log_pile(num_bottom_logs, log_subdiv, log_radius, log_length,
				log_position[0], log_position[1], log_position[2],
				log_rotation[0], log_rotation[1], log_rotation[2]);

		}


		if (ImGui::SliderFloat("Log Length", &log_length, 0.001, 0.8)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_log_pile_objects.size(); i++) {
				if (m_log_pile_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_log_pile_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_log_pile_objects.clear();
			log_pile(num_bottom_logs, log_subdiv, log_radius, log_length,
				log_position[0], log_position[1], log_position[2],
				log_rotation[0], log_rotation[1], log_rotation[2]);

		}
	}

	//---------- log pile end ------------------------------------------------



	//------ window start-------------------------------------------

	// window
	if (ImGui::Button("Window")) {
		is_window = true;
		window_initial_draw = -0.01;


	}
	if (is_window) {

		window_initial_draw += 0.01;
		if (window_initial_draw > -0.01 && window_initial_draw < 0.005) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_x_position, window_y_position, window_z_position,
				window_x_rotation, window_y_rotation, window_z_rotation,
				window_x_rotation_2, window_y_rotation_2, window_z_rotation_2,
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if ((ImGui::SliderFloat3("Window Position", window_position, -5.0, 5.0))) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if ((ImGui::SliderFloat3("Window Rotation", window_rotation_2, -360.0, 360.0))) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if (ImGui::SliderFloat("Window Width", &window_width, 0.001, 1.0)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if (ImGui::SliderFloat("Window Height", &window_height, 0.001, 0.7)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if (ImGui::SliderFloat("Window Depth", &window_depth, 0.001, 0.1)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if (ImGui::SliderFloat("Window Outer Trim Width", &window_outer_trim_width, 0.001, 0.3)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if (ImGui::SliderFloat("Window Outer Trim Depth", &window_outer_trim_depth, 0.001, 0.2)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if (ImGui::SliderFloat("Window Inner Trim Width", &window_inner_trim_width, 0.001, 0.3)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}

		if (ImGui::SliderFloat("Window Inner Trim Depth", &window_inner_trim_depth, 0.001, 0.2)) {
			// Deletion of objects after rendering
			for (int i = 0; i < m_window_objects.size(); i++) {
				if (m_window_objects[i] != nullptr) {     // check if current iteration is pointing to a null pointer
					delete m_window_objects[i];
				}
				else {
					cerr << "null pointer" << endl;
				}
			}
			// Clear the vector of pointers
			m_window_objects.clear();

			window(window_width, window_height, window_depth,
				window_position[0], window_position[1], window_position[2],
				window_rotation[0], window_rotation[1], window_rotation[2],
				window_rotation_2[0], window_rotation_2[1], window_rotation_2[2],
				window_outer_trim_width, window_outer_trim_depth,
				window_inner_trim_width, window_inner_trim_depth);
		}
	}


	//------ window end-------------------------------------------







	// --------------- Ryan's IMGUI end--------------------------


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




cgra::gl_mesh Application::torus(int theta_subdiv, int phi_subdiv, float theta_radius, float phi_radius,
	float x_position, float y_position, float z_position) {
	mesh_builder mb;
	mesh_vertex mv;

	glm::vec3 centroid(x_position, y_position, z_position);

	for (int i = 0; i <= theta_subdiv; i++) {  // <= accounts for the last row of vertices to complete the shape -> 5 lines creates 4 rows
		float curr_theta = ((2 * pi<float>()) / theta_subdiv) * i;
		for (int j = 0; j <= phi_subdiv; j++) {  // <= accounts for the last row of vertices to complete the shape -> 5 lines creates 4 rows
			float curr_phi = ((2 * pi<float>()) / phi_subdiv) * j;

			float tube_center_x = centroid.x + (theta_radius * sin(curr_theta));
			float tube_center_y = centroid.y;
			float tube_center_z = centroid.z + (theta_radius * sin(curr_theta));

			float tube_x_point = centroid.x + ((theta_radius + (phi_radius * cos(curr_phi))) * cos(curr_theta));
			float tube_y_point = centroid.y + (phi_radius * sin(curr_phi));
			float tube_z_point = centroid.z + ((theta_radius + (phi_radius * cos(curr_phi))) * sin(curr_theta));

			glm::vec3 curr_tube_point(tube_x_point, tube_y_point, tube_z_point);
			glm::vec3 curr_tube_center(tube_center_x, tube_center_y, tube_center_z);

			mv.pos = curr_tube_point;
			mv.norm = normalize(curr_tube_point - curr_tube_center);
			mb.push_vertex(mv);
		}
	}

	//-----push indices - start -----------------------------------------------------------
	// push each respective vertices index in proper order to generate triangles
	for (int i = 0; i < theta_subdiv; i++) {   // iterate to next column 
		for (int j = 0; j < phi_subdiv; j++) { // iterate up column creating each row

			// top left triangle for this respective row and column
			mb.push_index(j + (i * (phi_subdiv + 1)));
			mb.push_index(j + (i * (phi_subdiv + 1)) + 1);
			mb.push_index(j + (i * (phi_subdiv + 1)) + (phi_subdiv + 2));

			// bottom right triangle for this respective row and column
			mb.push_index(j + (i * (phi_subdiv + 1)));
			mb.push_index(j + (i * (phi_subdiv + 1)) + (phi_subdiv + 2));
			mb.push_index(j + (i * (phi_subdiv + 1)) + (phi_subdiv + 1));
		}
	}
	//-----push indices - end -----------------------------------------------------------

	m_model.mesh = mb.build();
	return m_model.mesh;
}





// I created these--------------------------------------------------------------------
cgra::gl_mesh Application::sphere_oblong(int theta_subdiv, int phi_subdiv, float radius, float phi_theta_radius_increment, float theta_radius_increment, float x_position, float y_position, float z_position, float x_rotation, float y_rotation, float z_rotation) {
	mesh_builder mb;
	mesh_vertex mv;


	// float phi_radius_increment = -0.5;      // TO DO: MAKE THIS INTO A PARAMETER!!!!!
	// float theta_radius_increment = 0.0;     // TO DO: MAKE THIS INTO A PARAMETER!!!!!

	float phi_radius = radius + phi_radius_increment;
	float theta_radius = radius + theta_radius_increment;


	glm::vec3 centroid(x_position, y_position, z_position);

	glm::vec3 BOTTOM_POLE(centroid.x, centroid.y - phi_radius, centroid.z);
	glm::vec3 TOP_POLE(centroid.x, centroid.y + phi_radius, centroid.z);

	int vert_count = 0;

	// iterate theta_subdiv x phi_subdiv to generate all vetices, then explicitly add the bottom pole, then top pole
	for (int i = 0; i <= theta_subdiv; i++) {  // <= accounts for the last column of vertices to complete the shape -> 5 lines creates 4 rows
		float curr_theta = ((2 * pi<float>()) / theta_subdiv) * i;
		float curr_theta_t_map = float(i) / float(theta_subdiv);
		for (int j = 1; j < phi_subdiv; j++) {  // skips 0 to skip bottom pole and < subdiv stops 1 subdiv from the end to skip the top pole
			float curr_phi = (((pi<float>()) / phi_subdiv) * j); // -(PI / 2.0);   // shift phi by - PI/2 so range is: [-PI/2, PI/2] to make vertical half-circle
			float curr_phi_t_map = float(j) / float(phi_subdiv);

			float x_point = centroid.x + (theta_radius * sin(curr_phi) * cos(curr_theta));
			float y_point = centroid.y + (phi_radius * cos(curr_phi));
			float z_point = centroid.z + (theta_radius * sin(curr_phi) * sin(curr_theta));

			glm::vec3 curr_point(x_point, y_point, z_point);

			mv.pos = curr_point;
			mv.norm = normalize(curr_point - centroid);
			mv.uv = vec2(curr_theta_t_map, curr_phi_t_map);

			mb.push_vertex(mv);

			vert_count++;
		}
	}

	// first bottom pole
	mv.pos = BOTTOM_POLE;
	mv.norm = BOTTOM_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);
	vert_count++;

	// first top pole
	mv.pos = TOP_POLE;
	mv.norm = TOP_POLE - centroid;
	mv.uv = vec2(0.0, 1.0);
	mb.push_vertex(mv);
	vert_count++;

	// duplicate top and bottom pole vertices for texture mapping purposes
	// last bottom pole
	mv.pos = BOTTOM_POLE;
	mv.norm = BOTTOM_POLE - centroid;
	mv.uv = vec2(1.0, 0.0);
	mb.push_vertex(mv);
	vert_count++;

	// last top pole
	mv.pos = TOP_POLE;
	mv.norm = TOP_POLE - centroid;
	mv.uv = vec2(1.0, 1.0);
	mb.push_vertex(mv);
	vert_count++;


	// ---- Rotation start ----------------------------
// convert degrees to radians
	float x_rotation_radians = glm::radians(x_rotation);
	float y_rotation_radians = glm::radians(y_rotation);
	float z_rotation_radians = glm::radians(z_rotation);

	glm::mat4 x_rotation_matrix = glm::rotate(glm::mat4(1.0f), x_rotation_radians, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 y_rotation_matrix = glm::rotate(glm::mat4(1.0f), y_rotation_radians, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 z_rotation_matrix = glm::rotate(glm::mat4(1.0f), z_rotation_radians, glm::vec3(0.0f, 0.0f, 1.0f));


	// Apply the rotation to each vertex
	for (cgra::mesh_vertex& vertex : mb.vertices) {
		vertex.pos -= centroid;   // bring position back to origin for rotation
		vertex.pos = glm::vec3(x_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(y_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(z_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos += centroid;   // return to current position
	}
	//----- Rotation end -------------------------


	//-----push indices - start -----------------------------------------------------------

	// push each respective vertices index in proper order to generate triangles
	for (int i = 0; i < theta_subdiv; i++) {   // iterate to next row  
		for (int j = 0; j < phi_subdiv - 2; j++) { // iterate up column

			// top left triangle for this respective row and column
			mb.push_index(j + (i * (phi_subdiv - 1)));
			mb.push_index(j + (i * (phi_subdiv - 1)) + 1);
			mb.push_index(j + (i * (phi_subdiv - 1)) + (phi_subdiv));

			// bottom right triangle for this respective row and column
			mb.push_index(j + (i * (phi_subdiv - 1)));
			mb.push_index(j + (i * (phi_subdiv - 1)) + (phi_subdiv));
			mb.push_index(j + (i * (phi_subdiv - 1)) + (phi_subdiv - 1));

		}
	}

	// draw bottom pole segments
	int index_interval = 0;
	int BOTTOM_POLE_INDEX = vert_count - 1;

	for (int i = 0; i < theta_subdiv; i++) {   // iterate to next row  

		// top left triangle for this respective row and column
		mb.push_index(index_interval);
		mb.push_index(index_interval + (phi_subdiv - 1));
		mb.push_index(BOTTOM_POLE_INDEX);

		index_interval += phi_subdiv - 1;
	}

	// draw top pole segments
	index_interval = phi_subdiv - 2;
	int TOP_POLE_INDEX = vert_count - 2;

	for (int i = 0; i < theta_subdiv; i++) {   // iterate to next row  

		// top left triangle for this respective row and column
		mb.push_index(index_interval);
		mb.push_index(index_interval + (phi_subdiv - 1));
		mb.push_index(TOP_POLE_INDEX);

		index_interval += phi_subdiv - 1;
	}

	//-----push indices - end -----------------------------------------------------------

	m_model.mesh = mb.build();
	return m_model.mesh;
}









// I created these--------------------------------------------------------------------
cgra::gl_mesh Application::cylinder(int subdiv, float top_radius, float bottom_radius, float height, bool is_cylinder_fill_top, bool is_cylinder_fill_bottom, float x_position, float y_position, float z_position, float x_rotation, float y_rotation, float z_rotation) {
	mesh_builder mb;
	mesh_vertex mv;

	float x_point;
	float y_point;
	float z_point;

	glm::vec3 centroid(x_position, y_position, z_position);

	glm::vec3 BOTTOM_POLE(centroid.x, centroid.y - (height * 0.5), centroid.z);
	glm::vec3 TOP_POLE(centroid.x, centroid.y + (height * 0.5), centroid.z);
	glm::vec3 RIGHT_POLE(centroid.x + ((top_radius + bottom_radius) * 0.5), centroid.y, centroid.z);
	glm::vec3 LEFT_POLE(centroid.x - ((top_radius + bottom_radius) * 0.5), centroid.y + (height * 0.5), centroid.z);

	int vert_count = 0;

	// iterate theta_subdiv x phi_subdiv to generate all vetices, then explicitly add the bottom pole, then top pole
	for (int i = 0; i < subdiv * 4; i++) {  // <= accounts for the last column of vertices to complete the shape -> 5 lines creates 4 rows
		float curr_theta = ((2 * pi<float>()) / subdiv) * i;

		if (i < subdiv) {  // bottom vertices for side
			x_point = centroid.x + (bottom_radius * cos(curr_theta));
			y_point = BOTTOM_POLE.y;
			z_point = centroid.z + (bottom_radius * sin(curr_theta));

			vec3 curr_point(x_point, y_point, z_point);
			mv.pos = curr_point;
			mv.norm = normalize(curr_point - centroid);
		}
		else if (i >= subdiv && i < (subdiv * 2)) {      // top vertices for side
			x_point = centroid.x + (top_radius * cos(curr_theta));
			y_point = TOP_POLE.y;
			z_point = centroid.z + (top_radius * sin(curr_theta));

			vec3 curr_point(x_point, y_point, z_point);
			mv.pos = curr_point;
			mv.norm = normalize(curr_point - centroid);
		}

		else if (i >= (subdiv * 2) && (i < subdiv * 3)) {  // bottom vertices for bottom
			x_point = centroid.x + (bottom_radius * cos(curr_theta));
			y_point = BOTTOM_POLE.y;
			z_point = centroid.z + (bottom_radius * sin(curr_theta));

			vec3 curr_point(x_point, y_point, z_point);
			mv.pos = curr_point;
			mv.norm = normalize(vec3(centroid.x, BOTTOM_POLE.y, centroid.z) - centroid);
		}
		else if (i >= (subdiv * 3)) {      // top vertices for top
			x_point = centroid.x + (top_radius * cos(curr_theta));
			y_point = TOP_POLE.y;
			z_point = centroid.z + (top_radius * sin(curr_theta));

			vec3 curr_point(x_point, y_point, z_point);
			mv.pos = curr_point;
			mv.norm = normalize(vec3(centroid.x, TOP_POLE.y, centroid.z) - centroid);
		}


		mb.push_vertex(mv);


	}

	// bottom pole
	mv.pos = BOTTOM_POLE;
	mv.norm = BOTTOM_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// top pole
	mv.pos = TOP_POLE;
	mv.norm = TOP_POLE - centroid;
	mv.uv = vec2(0.0, 1.0);
	mb.push_vertex(mv);


	// ---- Rotation start ----------------------------
// convert degrees to radians
	float x_rotation_radians = glm::radians(x_rotation);
	float y_rotation_radians = glm::radians(y_rotation);
	float z_rotation_radians = glm::radians(z_rotation);

	glm::mat4 x_rotation_matrix = glm::rotate(glm::mat4(1.0f), x_rotation_radians, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 y_rotation_matrix = glm::rotate(glm::mat4(1.0f), y_rotation_radians, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 z_rotation_matrix = glm::rotate(glm::mat4(1.0f), z_rotation_radians, glm::vec3(0.0f, 0.0f, 1.0f));


	// Apply the rotation to each vertex
	for (cgra::mesh_vertex& vertex : mb.vertices) {
		vertex.pos -= centroid;   // bring position back to origin for rotation
		vertex.pos = glm::vec3(x_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(y_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(z_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos += centroid;   // return to current position
	}
	//----- Rotation end -------------------------




	//-----push indices - start -----------------------------------------------------------

	int BOTTOM_POLE_INDEX = subdiv * 4;
	int TOP_POLE_INDEX = subdiv * 4 + 1;

	// push each respective vertices index in proper order to generate triangles
	for (int i = 0; i < subdiv; i++) {   // iterate through each row  
		if (i < subdiv - 1) {
			// top left triangle for this respective row and column
			mb.push_index(i);
			mb.push_index(i + subdiv);
			mb.push_index(i + subdiv + 1);

			// bottom right triangle for this respective row and column
			mb.push_index(i);
			mb.push_index(i + subdiv + 1);
			mb.push_index(i + 1);

			if (is_cylinder_fill_bottom) {
				// bottom triangle
				mb.push_index(i);
				mb.push_index(BOTTOM_POLE_INDEX);
				mb.push_index(i + 1);
			}

			if (is_cylinder_fill_top) {
				// top triangle
				mb.push_index(i + subdiv);
				mb.push_index(TOP_POLE_INDEX);
				mb.push_index(i + subdiv + 1);
			}
		}
		else {    // when i == subdiv; this is the last iteration so starts back at beginning vertex
			// top left triangle for this respective row and column
			mb.push_index(i);
			mb.push_index(i + subdiv);
			mb.push_index(i + 1);

			// bottom right triangle for this respective row and column
			mb.push_index(i);
			mb.push_index(i + 1);
			mb.push_index(0);

			if (is_cylinder_fill_bottom) {
				// bottom pole triangle 
				mb.push_index(i);
				mb.push_index(BOTTOM_POLE_INDEX);
				mb.push_index(0);
			}

			if (is_cylinder_fill_top) {
				// top pole triangle
				mb.push_index(i + subdiv);
				mb.push_index(TOP_POLE_INDEX);
				mb.push_index(i + 1);
			}
		}

	}


	//-----push indices - end -----------------------------------------------------------

	m_model.mesh = mb.build();

	return m_model.mesh;
}





void Application::tree(int tree_subdiv, float tree_height, float tree_top_radius, float tree_bottom_radius,   // tree parameters
	int branch_subdiv, int num_branch_rows, float branches_start, float branches_end, float branch_top_radius, float branch_bottom_radius,  // branch parameters
	float uniform_scale, float tree_x_position, float tree_y_position, float tree_z_position, float tree_x_rotation, float tree_y_rotation, float tree_z_rotation) {      // universal parameters
	// draw tree trunk
	bm_tree_object_ptr = new basic_model;
	// bm_object_ptr->mesh = cylinder_tree(tree_subdiv, tree_top_radius, tree_bottom_radius, tree_height, true, true, tree_x_position, tree_y_position, tree_z_position);

	bm_tree_object_ptr->mesh = cylinder(tree_subdiv, tree_top_radius, tree_bottom_radius, tree_height, true, true, tree_x_position, tree_y_position, tree_z_position, tree_x_rotation, tree_y_rotation, tree_z_rotation);

	bm_tree_object_ptr->color = vec3(0.45, 0.38, 0.3);    // assign color to object
	bm_tree_object_ptr->shader = just_shader;
	m_tree_objects.push_back(bm_tree_object_ptr);      // append current basic_model object

	// compute branch variables
	float branches_height = branches_start - branches_end;
	float branch_height_incr = branches_height / num_branch_rows;
	float branch_radius_incr = (branch_bottom_radius - branch_top_radius) / num_branch_rows;
	float tree_top = tree_height * 0.5;
	float tree_bottom = -tree_height * 0.5;



	// draw tree branches
	for (int i = 0; i < num_branch_rows; i++) {
		bm_tree_object_ptr = new basic_model;

		bm_tree_object_ptr->mesh = cylinder(branch_subdiv, 0, branch_top_radius + (branch_radius_incr * i), branch_height_incr, true, true,
			tree_x_position, tree_y_position + tree_top + branch_height_incr - (branch_height_incr * i), tree_z_position, tree_x_rotation, tree_y_rotation, tree_z_rotation);
		bm_tree_object_ptr->color = vec3(0.1, 0.7, 0);    // assign color to object
		bm_tree_object_ptr->shader = just_shader;
		m_tree_objects.push_back(bm_tree_object_ptr);      // append current basic_model object
	}

}






cgra::gl_mesh Application::rectangular_prism(float r_p_width, float r_p_height, float r_p_depth,
	float r_p_x_position, float r_p_y_position, float r_p_z_position,
	float r_p_x_rotation, float r_p_y_rotation, float r_p_z_rotation,
	float r_p_x_rotation_2, float r_p_y_rotation_2, float r_p_z_rotation_2,
	float r_p_uniform_scale) {
	mesh_builder mb;
	mesh_vertex mv;

	glm::vec3 centroid(r_p_x_position, r_p_y_position, r_p_z_position);

	glm::vec3 BOTTOM_POLE(centroid.x, centroid.y - (r_p_height * 0.5), centroid.z);
	glm::vec3 TOP_POLE(centroid.x, centroid.y + (r_p_height * 0.5), centroid.z);
	glm::vec3 RIGHT_POLE(centroid.x + r_p_width * 0.5, centroid.y, centroid.z);
	glm::vec3 LEFT_POLE(centroid.x - r_p_width * 0.5, centroid.y, centroid.z);
	glm::vec3 FRONT_POLE(centroid.x, centroid.y, centroid.z + r_p_depth * 0.5);
	glm::vec3 BACK_POLE(centroid.x, centroid.y, centroid.z - r_p_depth * 0.5);

	// define vertices----------------------------------------------------------
	// -----------------------
	// vertex 0: (-1,-1,1)   // bottom 
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BOTTOM_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 1: (-1,-1,-1)   // bottom 
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BOTTOM_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 2: (1,-1,-1)   // bottom 
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BOTTOM_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 3: (1,-1,1)   // bottom 
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BOTTOM_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);
	//-------------------------
	// vertex 4: (-1,1,1)    // top
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = TOP_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 5: (-1,1,-1)    // top
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = TOP_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 6: (1,1,-1)    // top
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = TOP_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 7: (1,1,1)    // top
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = TOP_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);
	//------------------
	// vertex 8: (1,-1,1)    // right
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = RIGHT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 9: (1,1,1)    // right
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = RIGHT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 10: (1,1,-1)    // right
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = RIGHT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 11: (1,-1,-1)    // right
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = RIGHT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);
	//-------------------
	// vertex 12: (-1,-1,1)    // left
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = LEFT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 13: (-1,1,-1)    // left
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = LEFT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 14: (-1,1,1)    // left
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = LEFT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 15: (-1,-1,1)    // left
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = LEFT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);
	//----------------------
	// vertex 16: (-1,-1,1)    // front
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = FRONT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 17: (-1,1,1)    // front
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = FRONT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 18: (1,1,1)    // front
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = FRONT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 19: (1,-1,1)    // front
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z + r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = FRONT_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);
	//------------------------
	// vertex 20: (1,-1,-1)    // back
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BACK_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 21: (1,1,-1)    // back
	mv.pos = vec3(centroid.x + r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BACK_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 22: (-1,1,-1)    // back
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y + r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BACK_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);

	// vertex 23: (-1,-1,-1)    // back
	mv.pos = vec3(centroid.x - r_p_width * 0.5 * r_p_uniform_scale, centroid.y - r_p_height * 0.5 * r_p_uniform_scale, centroid.z - r_p_depth * 0.5 * r_p_uniform_scale);
	mv.norm = BACK_POLE - centroid;
	mv.uv = vec2(0.0, 0.0);
	mb.push_vertex(mv);


	// ---- Rotation start ----------------------------
	// convert degrees to radians
	float r_p_x_rotation_radians = glm::radians(r_p_x_rotation);
	float r_p_y_rotation_radians = glm::radians(r_p_y_rotation);
	float r_p_z_rotation_radians = glm::radians(r_p_z_rotation);

	glm::mat4 r_p_x_rotation_matrix = glm::rotate(glm::mat4(1.0f), r_p_x_rotation_radians, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 r_p_y_rotation_matrix = glm::rotate(glm::mat4(1.0f), r_p_y_rotation_radians, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 r_p_z_rotation_matrix = glm::rotate(glm::mat4(1.0f), r_p_z_rotation_radians, glm::vec3(0.0f, 0.0f, 1.0f));


	// Apply the rotation to each vertex
	for (cgra::mesh_vertex& vertex : mb.vertices) {
		vertex.pos -= centroid;   // bring position back to origin for rotation
		vertex.pos = glm::vec3(r_p_x_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(r_p_y_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(r_p_z_rotation_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos += centroid;   // return to current position
	}
	//----- Rotation end -------------------------


	// ---- Rotation_2 start ----------------------------
	// convert degrees to radians
	float r_p_x_rotation_2_radians = glm::radians(r_p_x_rotation_2);
	float r_p_y_rotation_2_radians = glm::radians(r_p_y_rotation_2);
	float r_p_z_rotation_2_radians = glm::radians(r_p_z_rotation_2);

	glm::mat4 r_p_x_rotation_2_matrix = glm::rotate(glm::mat4(1.0f), r_p_x_rotation_2_radians, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 r_p_y_rotation_2_matrix = glm::rotate(glm::mat4(1.0f), r_p_y_rotation_2_radians, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 r_p_z_rotation_2_matrix = glm::rotate(glm::mat4(1.0f), r_p_z_rotation_2_radians, glm::vec3(0.0f, 0.0f, 1.0f));


	// Apply the rotation_2 to each vertex
	for (cgra::mesh_vertex& vertex : mb.vertices) {
		//vertex.pos -= centroid;   // bring position back to origin for rotation
		vertex.pos = glm::vec3(r_p_x_rotation_2_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(r_p_y_rotation_2_matrix * glm::vec4(vertex.pos, 1.0f));
		vertex.pos = glm::vec3(r_p_z_rotation_2_matrix * glm::vec4(vertex.pos, 1.0f));
		//vertex.pos += centroid;   // return to current position
	}
	//----- Rotation 2 end -------------------------




	//-----push indices - start -----------------------------------------------------------
	// (0,1,2), (0,2,3)    // bottom

	for (int i = 0; i < 24; i += 4) {
		mb.push_index(i);
		mb.push_index(i + 1);
		mb.push_index(i + 2);

		mb.push_index(i);
		mb.push_index(i + 2);
		mb.push_index(i + 3);
	}



	return mb.build();
}



void Application::table(float table_top_width, float table_top_height, float table_top_depth,
	float table_x_position, float table_y_position, float table_z_position,
	float table_x_rotation, float table_y_rotation, float table_z_rotation,
	float table_x_rotation_2, float table_y_rotation_2, float table_z_rotation_2,
	float table_leg_width_depth, float table_leg_height, float table_uniform_scale) {

	// draw table top
	bm_table_object_ptr = new basic_model;  // declare basic_model pointer to allocate on the heap

	bm_table_object_ptr->mesh = rectangular_prism(            // create the mesh
		table_top_width, table_top_height, table_top_depth,
		table_x_position, table_y_position, table_z_position,
		table_x_rotation, table_y_rotation, table_z_rotation,
		table_x_rotation_2, table_y_rotation_2, table_z_rotation_2,
		table_uniform_scale
	);
	bm_table_object_ptr->color = vec3(0.44, 0.18, 0.005);    // assign color to object
	bm_table_object_ptr->shader = just_shader;    // assign shader to object

	// bm_object_ptr is a basic_model*; declared application.hpp
	// initializes current basic_model pointer with the heap allocated basic_model object that is initialized with the current gl::mesh object, called mesh_object

	// m_all_objects is a vector<basic_model*>; declared application.hpp
	// stores all basic_model pointers to heap allocated basic_model objects
	m_table_objects.push_back(bm_table_object_ptr);      // append current basic_model object



	// draw 4 legs---------------------------------
	// leg 1 (+,-,+)---------------------------------
	bm_table_object_ptr = new basic_model;
	bm_table_object_ptr->mesh = rectangular_prism(table_leg_width_depth, table_leg_height, table_leg_width_depth,     // create the mesh
		table_x_position + (table_top_width * 0.5) - (table_leg_width_depth * 0.5),
		table_y_position - (table_leg_height * 0.5) - (table_top_height * 0.5),
		table_z_position + (table_top_depth * 0.5) - (table_leg_width_depth * 0.5),
		table_x_rotation, table_y_rotation, table_z_rotation,
		table_x_rotation_2, table_y_rotation_2, table_z_rotation_2, table_uniform_scale);
	bm_table_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_table_object_ptr->shader = just_shader;    // assign shader to object
	m_table_objects.push_back(bm_table_object_ptr);      // append current basic_model object


	// leg 2 (-,-,+)---------------------------------
	bm_table_object_ptr = new basic_model;
	bm_table_object_ptr->mesh = rectangular_prism(table_leg_width_depth, table_leg_height, table_leg_width_depth,     // create the mesh
		table_x_position - (table_top_width * 0.5) + (table_leg_width_depth * 0.5),
		table_y_position - (table_leg_height * 0.5) - (table_top_height * 0.5),
		table_z_position + (table_top_depth * 0.5) - (table_leg_width_depth * 0.5),
		table_x_rotation, table_y_rotation, table_z_rotation,
		table_x_rotation_2, table_y_rotation_2, table_z_rotation_2, table_uniform_scale);

	bm_table_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_table_object_ptr->shader = just_shader;    // assign shader to object
	m_table_objects.push_back(bm_table_object_ptr);       // append current basic_model object

	// leg 3 (-,-,-)---------------------------------
	bm_table_object_ptr = new basic_model;
	bm_table_object_ptr->mesh = rectangular_prism(table_leg_width_depth, table_leg_height, table_leg_width_depth,      // create the mesh
		table_x_position - (table_top_width * 0.5) + (table_leg_width_depth * 0.5),
		table_y_position - (table_leg_height * 0.5) - (table_top_height * 0.5),
		table_z_position - (table_top_depth * 0.5) + (table_leg_width_depth * 0.5),
		table_x_rotation, table_y_rotation, table_z_rotation,
		table_x_rotation_2, table_y_rotation_2, table_z_rotation_2, table_uniform_scale);

	bm_table_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_table_object_ptr->shader = just_shader;    // assign shader to object
	m_table_objects.push_back(bm_table_object_ptr);      // append current basic_model object

	// leg 4 (+,-,-)---------------------------------
	bm_table_object_ptr = new basic_model;
	bm_table_object_ptr->mesh = rectangular_prism(table_leg_width_depth, table_leg_height, table_leg_width_depth,      // create the mesh
		table_x_position + (table_top_width * 0.5) - (table_leg_width_depth * 0.5),
		table_y_position - (table_leg_height * 0.5) - (table_top_height * 0.5),
		table_z_position - (table_top_depth * 0.5) + (table_leg_width_depth * 0.5),
		table_x_rotation, table_y_rotation, table_z_rotation,
		table_x_rotation_2, table_y_rotation_2, table_z_rotation_2, table_uniform_scale);

	bm_table_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_table_object_ptr->shader = just_shader;    // assign shader to object
	m_table_objects.push_back(bm_table_object_ptr);       // append current basic_model object

	//------------------------------------------------

}





void Application::chair(float seat_width, float seat_height, float seat_depth,
	float seat_x_position, float seat_y_position, float seat_z_position,
	float seat_x_rotation, float seat_y_rotation, float seat_z_rotation,
	float seat_x_rotation_2, float seat_y_rotation_2, float seat_z_rotation_2,
	float leg_width_depth, float leg_height,
	float b_support_height, float b_support_rest_height, float chair_uniform_scale) {

	// draw seat
	bm_chair_object_ptr = new basic_model;  // declare basic_model pointer to allocate on the heap

	bm_chair_object_ptr->mesh = rectangular_prism(            // create the mesh
		seat_width, seat_height, seat_depth,
		seat_x_position, seat_y_position, seat_z_position,
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2,
		chair_uniform_scale
	);
	bm_chair_object_ptr->color = vec3(0.55, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object

	// bm_object_ptr is a basic_model*; declared application.hpp
	// initializes current basic_model pointer with the heap allocated basic_model object that is initialized with the current gl::mesh object, called mesh_object

	// m_all_objects is a vector<basic_model*>; declared application.hpp
	// stores all basic_model pointers to heap allocated basic_model objects
	m_chair_objects.push_back(bm_chair_object_ptr);      // append current basic_model object



	// draw 4 legs---------------------------------
	// leg 1 (+,-,+)---------------------------------
	bm_chair_object_ptr = new basic_model;
	bm_chair_object_ptr->mesh = rectangular_prism(leg_width_depth, leg_height, leg_width_depth,     // create the mesh
		seat_x_position + (seat_width * 0.5) - (leg_width_depth * 0.5),
		seat_y_position - (leg_height * 0.5) - (seat_height * 0.5),
		seat_z_position + (seat_depth * 0.5) - (leg_width_depth * 0.5),
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2, chair_uniform_scale);
	bm_chair_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object
	m_chair_objects.push_back(bm_chair_object_ptr);      // append current basic_model object


	// leg 2 (-,-,+)---------------------------------
	bm_chair_object_ptr = new basic_model;
	bm_chair_object_ptr->mesh = rectangular_prism(leg_width_depth, leg_height, leg_width_depth,     // create the mesh
		seat_x_position - (seat_width * 0.5) + (leg_width_depth * 0.5),
		seat_y_position - (leg_height * 0.5) - (seat_height * 0.5),
		seat_z_position + (seat_depth * 0.5) - (leg_width_depth * 0.5),
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2, chair_uniform_scale);

	bm_chair_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object
	m_chair_objects.push_back(bm_chair_object_ptr);       // append current basic_model object

	// leg 3 (-,-,-)---------------------------------
	bm_chair_object_ptr = new basic_model;
	bm_chair_object_ptr->mesh = rectangular_prism(leg_width_depth, leg_height, leg_width_depth,      // create the mesh
		seat_x_position - (seat_width * 0.5) + (leg_width_depth * 0.5),
		seat_y_position - (leg_height * 0.5) - (seat_height * 0.5),
		seat_z_position - (seat_depth * 0.5) + (leg_width_depth * 0.5),
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2, chair_uniform_scale);

	bm_chair_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object
	m_chair_objects.push_back(bm_chair_object_ptr);      // append current basic_model object

	// leg 4 (+,-,-)---------------------------------
	bm_chair_object_ptr = new basic_model;
	bm_chair_object_ptr->mesh = rectangular_prism(leg_width_depth, leg_height, leg_width_depth,      // create the mesh
		seat_x_position + (seat_width * 0.5) - (leg_width_depth * 0.5),
		seat_y_position - (leg_height * 0.5) - (seat_height * 0.5),
		seat_z_position - (seat_depth * 0.5) + (leg_width_depth * 0.5),
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2, chair_uniform_scale);

	bm_chair_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object
	m_chair_objects.push_back(bm_chair_object_ptr);       // append current basic_model object

	//------------------------------------------------


	// draw back support beams
	// leg 5 (+,-,-)---------------------------------
	bm_chair_object_ptr = new basic_model;
	bm_chair_object_ptr->mesh = rectangular_prism(leg_width_depth, b_support_height, leg_width_depth,    // create the mesh
		seat_x_position + (seat_width * 0.5) - (leg_width_depth * 0.5),
		seat_y_position + (b_support_height * 0.5) + (seat_height * 0.5),
		seat_z_position - (seat_depth * 0.5) + (leg_width_depth * 0.5),
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2, chair_uniform_scale);

	bm_chair_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object
	m_chair_objects.push_back(bm_chair_object_ptr);       // append current basic_model object

	// leg 6 (-,-,-)---------------------------------
	bm_chair_object_ptr = new basic_model;
	bm_chair_object_ptr->mesh = rectangular_prism(leg_width_depth, b_support_height, leg_width_depth,     // create the mesh
		seat_x_position - (seat_width * 0.5) + (leg_width_depth * 0.5),
		seat_y_position + (b_support_height * 0.5) + (seat_height * 0.5),
		seat_z_position - (seat_depth * 0.5) + (leg_width_depth * 0.5),
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2, chair_uniform_scale);

	bm_chair_object_ptr->color = vec3(0.05, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object
	m_chair_objects.push_back(bm_chair_object_ptr);       // append current basic_model object

	// back rest 7 (0,-,-)---------------------------------
	bm_chair_object_ptr = new basic_model;
	bm_chair_object_ptr->mesh = rectangular_prism(seat_width - leg_width_depth, b_support_rest_height, leg_width_depth * 0.7,     // create the mesh
		seat_x_position,
		seat_y_position + (b_support_height * 0.5) + (seat_height * 0.5),
		seat_z_position - (seat_depth * 0.5) + (leg_width_depth * 0.5),
		seat_x_rotation, seat_y_rotation, seat_z_rotation,
		seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2, chair_uniform_scale);

	bm_chair_object_ptr->color = vec3(0.55, 0.05, 0.05);    // assign color to object
	bm_chair_object_ptr->shader = just_shader;    // assign shader to object
	m_chair_objects.push_back(bm_chair_object_ptr);       // append current basic_model object


}





void Application::fire_guard(float fire_guard_radius, int fire_guard_subdiv, int num_brick_rows,
	float brick_width, float brick_height, float brick_depth,
	float fire_guard_x_position, float fire_guard_y_position, float fire_guard_z_position,
	float fire_guard_x_rotation, float fire_guard_y_rotation, float fire_guard_z_rotation) {
	// draw seat----------------------------------

	vec3 centroid(fire_guard_x_position, fire_guard_y_position, fire_guard_z_position);

	float radian_incr = 2 * pi<float>() / fire_guard_subdiv;

	for (int i = 0; i < num_brick_rows; i++) {
		for (int j = 0; j < fire_guard_subdiv; j++) {
			if (i % 2 == 0) {      // if is an even row
				vec3 curr_point = vec3(centroid.x + fire_guard_radius,
					centroid.y + (brick_height * 0.5) + (brick_height * i),
					centroid.z);

				bm_fire_guard_object_ptr = new basic_model;
				bm_fire_guard_object_ptr->mesh = rectangular_prism(brick_width, brick_height, brick_depth,
					curr_point.x, curr_point.y, curr_point.z,
					0, degrees(radian_incr * 0.5) * j, 0, 0, degrees(radian_incr) * j, 0, 1);      // I hard coded rotation_2 values and uniform scale value!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				bm_fire_guard_object_ptr->color = vec3(0.3, 0.3, 0.3);    // assign color to object
				bm_fire_guard_object_ptr->shader = just_shader;    // assign shader to object
				m_fire_guard_objects.push_back(bm_fire_guard_object_ptr);      // append current basic_model object
			}
			else {          // if is odd row
				vec3 curr_point = vec3(centroid.x + fire_guard_radius,
					centroid.y + (brick_height * 0.5) + (brick_height * i),
					centroid.z);

				bm_fire_guard_object_ptr = new basic_model;
				bm_fire_guard_object_ptr->mesh = rectangular_prism(brick_width, brick_height, brick_depth,
					curr_point.x, curr_point.y, curr_point.z,
					0, degrees(radian_incr * 0.5) * j, 0, 0, degrees(radian_incr) * j + degrees(radian_incr * 0.5), 0, 1);      // I hard coded rotation_2 values and uniform scale value!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				bm_fire_guard_object_ptr->color = vec3(0.3, 0.3, 0.3);    // assign color to object
				bm_fire_guard_object_ptr->shader = just_shader;    // assign shader to object
				m_fire_guard_objects.push_back(bm_fire_guard_object_ptr);      // append current basic_model object
			}
		}
	}


}




void Application::log_pile(int num_bottom_logs, int log_subdiv, float log_radius, float log_length,
	float log_x_position, float log_y_position, float log_z_position,
	float log_x_rotation, float log_y_rotation, float log_z_rotation) {

	vec3 centroid(log_x_position, log_y_position, log_z_position);

	float curr_x_position = centroid.x;
	float curr_y_position = centroid.y + log_radius;
	float curr_z_position = centroid.z;


	for (int i = num_bottom_logs; i > 0; i--) {
		int log_x_pos_offset = i / 2;
		if (i % 2 == 1) {   // if row has odd number of logs
			// draw leftside logs
			for (int j = 0; j < log_x_pos_offset; j++) {
				bm_log_pile_object_ptr = new basic_model;
				bm_log_pile_object_ptr->mesh = cylinder(log_subdiv, log_radius, log_radius, log_length, true, true, curr_x_position - (log_radius * 2 * (log_x_pos_offset - j)), curr_y_position, curr_z_position, 90, log_y_rotation, log_z_rotation);
				bm_log_pile_object_ptr->color = vec3(0.5, 0.4, 0.35);    // assign color to object
				bm_log_pile_object_ptr->shader = just_shader;    // assign shader to object
				m_log_pile_objects.push_back(bm_log_pile_object_ptr);      // append current basic_model object
			}
			// draw center log
			bm_log_pile_object_ptr = new basic_model;
			bm_log_pile_object_ptr->mesh = cylinder(log_subdiv, log_radius, log_radius, log_length, true, true, curr_x_position, curr_y_position, curr_z_position, 90, log_y_rotation, log_z_rotation);
			bm_log_pile_object_ptr->color = vec3(0.5, 0.4, 0.35);    // assign color to object
			bm_log_pile_object_ptr->shader = just_shader;    // assign shader to object
			m_log_pile_objects.push_back(bm_log_pile_object_ptr);      // append current basic_model object

			// draw rightside logs
			for (int j = 0; j < log_x_pos_offset; j++) {
				bm_log_pile_object_ptr = new basic_model;
				bm_log_pile_object_ptr->mesh = cylinder(log_subdiv, log_radius, log_radius, log_length, true, true, curr_x_position + (log_radius * 2 * (log_x_pos_offset - j)), curr_y_position, curr_z_position, 90, log_y_rotation, log_z_rotation);
				bm_log_pile_object_ptr->color = vec3(0.5, 0.4, 0.35);    // assign color to object
				bm_log_pile_object_ptr->shader = just_shader;    // assign shader to object
				m_log_pile_objects.push_back(bm_log_pile_object_ptr);      // append current basic_model object
			}
			curr_y_position += log_radius * 1.66;
		}
		else if (i % 2 == 0) {  // if row has even number of logs
			// draw leftside logs
			for (int j = 0; j < log_x_pos_offset; j++) {
				bm_log_pile_object_ptr = new basic_model;
				bm_log_pile_object_ptr->mesh = cylinder(log_subdiv, log_radius, log_radius, log_length, true, true, curr_x_position - (log_radius * 2 * (log_x_pos_offset - j)) + log_radius, curr_y_position, curr_z_position, 90, log_y_rotation, log_z_rotation);
				bm_log_pile_object_ptr->color = vec3(0.5, 0.4, 0.35);    // assign color to object
				bm_log_pile_object_ptr->shader = just_shader;    // assign shader to object
				m_log_pile_objects.push_back(bm_log_pile_object_ptr);      // append current basic_model object
			}
			// draw rightside logs
			for (int j = 0; j < log_x_pos_offset; j++) {
				bm_log_pile_object_ptr = new basic_model;
				bm_log_pile_object_ptr->mesh = cylinder(log_subdiv, log_radius, log_radius, log_length, true, true, curr_x_position + (log_radius * 2 * (log_x_pos_offset - j)) - log_radius, curr_y_position, curr_z_position, 90, log_y_rotation, log_z_rotation);
				bm_log_pile_object_ptr->color = vec3(0.5, 0.4, 0.35);    // assign color to object
				bm_log_pile_object_ptr->shader = just_shader;    // assign shader to object
				m_log_pile_objects.push_back(bm_log_pile_object_ptr);      // append current basic_model object
			}
			curr_y_position += log_radius * 1.66;
		}
	}

}




void Application::window(float window_width, float window_height, float window_depth,
	float window_x_position, float window_y_position, float window_z_position,
	float window_x_rotation, float window_y_rotation, float window_z_rotation,
	float window_x_rotation_2, float window_y_rotation_2, float window_z_rotation_2,
	float outer_trim_width, float outer_trim_depth,
	float inner_trim_width, float inner_trim_depth) {


	// draw glass
	bm_window_object_ptr = new basic_model;
	bm_window_object_ptr->mesh = rectangular_prism(window_width - (window_outer_trim_width), window_height - (window_outer_trim_width), window_depth,
		window_x_position, window_y_position, window_z_position,
		window_x_rotation, window_y_rotation, window_z_rotation,
		window_x_rotation_2, window_y_rotation_2, window_z_rotation_2, window_uniform_scale);
	bm_window_object_ptr->color = vec3(0.36, 0.43, 0.58);    // assign color to object
	bm_window_object_ptr->shader = just_shader;
	m_window_objects.push_back(bm_window_object_ptr);      // append current basic_model object



	// ----- draw 4 outer trim -----------
	// bottom outer trim
	bm_window_object_ptr = new basic_model;
	bm_window_object_ptr->mesh = rectangular_prism(window_width, window_outer_trim_width, window_outer_trim_depth,
		window_x_position, window_y_position - (window_height * 0.5), window_z_position,
		window_x_rotation, window_y_rotation, window_z_rotation,
		window_x_rotation_2, window_y_rotation_2, window_z_rotation_2, window_uniform_scale);
	bm_window_object_ptr->color = vec3(0.1, 0.1, 0.1);    // assign color to object
	bm_window_object_ptr->shader = just_shader;
	m_window_objects.push_back(bm_window_object_ptr);      // append current basic_model object


	// top outer trim
	bm_window_object_ptr = new basic_model;
	bm_window_object_ptr->mesh = rectangular_prism(window_width, window_outer_trim_width, window_outer_trim_depth,
		window_x_position, window_y_position + (window_height * 0.5), window_z_position,
		window_x_rotation, window_y_rotation, window_z_rotation,
		window_x_rotation_2, window_y_rotation_2, window_z_rotation_2, window_uniform_scale);
	bm_window_object_ptr->color = vec3(0.1, 0.1, 0.1);    // assign color to object
	bm_window_object_ptr->shader = just_shader;
	m_window_objects.push_back(bm_window_object_ptr);      // append current basic_model object

	// right outer trim
	bm_window_object_ptr = new basic_model;
	bm_window_object_ptr->mesh = rectangular_prism(outer_trim_width, window_height + outer_trim_width, outer_trim_depth,
		window_x_position + (window_width * 0.5), window_y_position, window_z_position,
		window_x_rotation, window_y_rotation, window_z_rotation,
		window_x_rotation_2, window_y_rotation_2, window_z_rotation_2, window_uniform_scale);
	bm_window_object_ptr->color = vec3(0.1, 0.1, 0.1);    // assign color to object
	bm_window_object_ptr->shader = just_shader;
	m_window_objects.push_back(bm_window_object_ptr);      // append current basic_model object

	// left outer trim
	bm_window_object_ptr = new basic_model;
	bm_window_object_ptr->mesh = rectangular_prism(outer_trim_width, window_height + outer_trim_width, outer_trim_depth,
		window_x_position - (window_width * 0.5), window_y_position, window_z_position,
		window_x_rotation, window_y_rotation, window_z_rotation,
		window_x_rotation_2, window_y_rotation_2, window_z_rotation_2, window_uniform_scale);
	bm_window_object_ptr->color = vec3(0.1, 0.1, 0.1);    // assign color to object
	bm_window_object_ptr->shader = just_shader;
	m_window_objects.push_back(bm_window_object_ptr);      // append current basic_model object

	// draw inner trim
	// vertical inner trim
	bm_window_object_ptr = new basic_model;
	bm_window_object_ptr->mesh = rectangular_prism(inner_trim_width, window_height - (outer_trim_width), inner_trim_depth,
		window_x_position, window_y_position, window_z_position,
		window_x_rotation, window_y_rotation, window_z_rotation,
		window_x_rotation_2, window_y_rotation_2, window_z_rotation_2, window_uniform_scale);
	bm_window_object_ptr->color = vec3(0.1, 0.1, 0.1);    // assign color to object
	bm_window_object_ptr->shader = just_shader;
	m_window_objects.push_back(bm_window_object_ptr);      // append current basic_model object

	// horizontal inner trim
	bm_window_object_ptr = new basic_model;
	bm_window_object_ptr->mesh = rectangular_prism(window_width, inner_trim_width, inner_trim_depth,
		window_x_position, window_y_position, window_z_position,
		window_x_rotation, window_y_rotation, window_z_rotation,
		window_x_rotation_2, window_y_rotation_2, window_z_rotation_2, window_uniform_scale);
	bm_window_object_ptr->color = vec3(0.1, 0.1, 0.1);    // assign color to object
	bm_window_object_ptr->shader = just_shader;
	m_window_objects.push_back(bm_window_object_ptr);      // append current basic_model object


}



