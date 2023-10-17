
#pragma once

// glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


// project
#include "opengl.hpp"
#include "particle_system.hpp"
#include "cgra/cgra_mesh.hpp"


using namespace glm;
using namespace std;

// depth stuff
static float mapnear = 1.0f;
static float mapfar = 50.0f;
static float mapdepth = 15.0f;
static bool depthMode = false;
static int searchRegion = 4;
static int wWidth;
static int wHeight;

struct basic_model {
	GLuint shader = 3;
	cgra::gl_mesh mesh;
	glm::vec3 color{ 0.7 };

	//shader parameters
	float scale = 5;
	float heightScale = 0.025;
	float tilingScale = 2;
	float POMmaxLayers = 32;

	


	void draw(const glm::mat4& view, const glm::mat4 proj, const glm::vec3& position, const float rotationAngle, const glm::vec3& rotationAxis, GLint diff, GLint normal, GLint height);
};


// Main application class
//
class Application {
private:
	// window
	glm::vec2 m_windowsize;
	GLFWwindow *m_window;

	// oribital camera
	float m_pitch = .11;
	float m_yaw = -0.21;
	float m_distance = 2.66;

	// depth stuff
	int mode = 0;

	//------ Ryan's parameters start--------------------
	GLuint just_shader;
	GLuint jamie_shader;

	Particle_System p_system;

	float sphere_initial_draw = -0.01;
	float cube_initial_draw = -0.01;
	float cube_sphere_initial_draw = -0.01;
	float torus_initial_draw = -0.01;
	//----------------------------------
	float sphere_oblong_initial_draw = -0.01;
	float cylinder_initial_draw = -0.01;
	float cylinder_extrude_initial_draw = -0.01;
	//-----------------------------------------------
	float tree_initial_draw = -0.01;
	//------------------
	float rectangular_prism_initial_draw = -0.01;
	float chair_initial_draw = -0.01;
	float fire_guard_initial_draw = -0.01;
	float window_initial_draw = -0.01;
	
	
	
	bool is_torus = false;
	int subdiv = 8;
	int theta_subdiv = 16;
	int phi_subdiv = 8;
	float radius = 1.0;
	float theta_radius = 1.0;
	float phi_radius = 0.25;
	float x_position = 0.0;
	float y_position = 0.0;
	float z_position = 0.0;
	float position[3] = { x_position, y_position, z_position };
	float x_rotation = 0.0;
	float y_rotation = 0.0;
	float z_rotation = 0.0;
	float rotation[3] = { x_rotation, y_rotation, z_rotation };

	bool is_sphere_oblong = false;
	float phi_radius_increment = 0.0;
	float theta_radius_increment = 0.0;


	bool is_cylinder = false;
	float top_radius = 1.0;
	float bottom_radius = 1.0;
	float width = 1.0;
	float height = 1.0;
	float depth = 1.0;
	bool is_cylinder_fill_top = true;
	bool is_cylinder_fill_bottom = true;

	float top_inner_radius = 1.0;
	float bottom_inner_radius = 1.0;

	float top_outer_radius = 1.2;
	float bottom_outer_radius = 1.2;


	// tree parameters start----------------------
	bool is_tree = true;
	int tree_subdiv = 12;
	float tree_height = 2.57;
	float tree_top_radius = 0.036;
	float tree_bottom_radius = 0.124;
	// branch parameters
	int branch_subdiv = 7;
	int num_branch_rows = 18;
	float branches_start = 1.9;
	float branches_end = 0.001;
	float branch_top_radius = 0.05;
	float branch_bottom_radius = 0.5;
	float uniform_scale = 1.0;
	float tree_x_position = -1.8;
	float tree_y_position = 0.85;
	float tree_z_position = -3.6;
	float tree_position[3] = { tree_x_position, tree_y_position, tree_z_position };

	//
	bool is_rectangular_prism = false;
	float tree_x_rotation = 0.0;
	float tree_y_rotation = 0.0;
	float tree_z_rotation = 0.0;
	float tree_rotation[3] = { tree_x_rotation, tree_y_rotation, tree_z_rotation };
	// tree parameters end--------------------------------


	// ------ rectangular prism start-----------------------------
	float r_p_width = 0.4;
	float r_p_height = 0.05;
	float r_p_depth = 0.5;
	float r_p_x_position = 0.0;
	float r_p_y_position = 0.0;
	float r_p_z_position = 0.0;
	float r_p_position[3] = { r_p_x_position, r_p_y_position, r_p_z_position };
	float r_p_x_rotation = 0.0;
	float r_p_y_rotation = 0.0;
	float r_p_z_rotation = 0.0;
	float r_p_rotation[3] = { r_p_x_rotation, r_p_y_rotation, r_p_z_rotation };
	float r_p_x_rotation_2 = 0.0;
	float r_p_y_rotation_2 = 0.0;
	float r_p_z_rotation_2 = 0.0;
	float r_p_rotation_2[3] = { r_p_x_rotation_2, r_p_y_rotation_2, r_p_z_rotation_2 };
	float r_p_uniform_scale = 1;
	// --------- rectangular prism end ----------------------------------

	// ----chair start --------------------------
	bool is_chair = true;
	float seat_width = 1.2;
	float seat_height = 0.02;
	float seat_depth = 0.25;
	float seat_x_position = 0.0;
	float seat_y_position = 0.0;
	float seat_z_position = -1.4;
	float seat_position[3] = { seat_x_position, seat_y_position, seat_z_position };
	float seat_x_rotation = 0.0;
	float seat_y_rotation = 0.0;
	float seat_z_rotation = 0.0;
	float seat_rotation[3] = { seat_x_rotation, seat_y_rotation, seat_z_rotation };
	float seat_x_rotation_2 = 0.0;
	float seat_y_rotation_2 = -100.0;
	float seat_z_rotation_2 = 0.0;
	float seat_rotation_2[3] = { seat_x_rotation_2, seat_y_rotation_2, seat_z_rotation_2 };
	float leg_width_depth = 0.02;
	float leg_height = 0.25;
	float b_support_height = 0.275;
	float b_support_rest_height = 0.17;
	float chair_uniform_scale = 1;
	// ----chair end-------------------


	// ----table start --------------------------
	bool is_table = true;
	float table_initial_draw = -0.01;
	float table_top_width = 0.363;
	float table_top_height = 0.026;
	float table_top_depth = 0.962;
	float table_x_position = -1.42;
	float table_y_position = 0.19;
	float table_z_position = 0.1;
	float table_position[3] = { table_x_position, table_y_position, table_z_position };
	float table_x_rotation = 0.0;
	float table_y_rotation = 0.0;
	float table_z_rotation = 0.0;
	float table_rotation[3] = { table_x_rotation, table_y_rotation, table_z_rotation };
	float table_x_rotation_2 = 0.0;
	float table_y_rotation_2 = 53.6;
	float table_z_rotation_2 = 0.0;
	float table_rotation_2[3] = { table_x_rotation_2, table_y_rotation_2, table_z_rotation_2 };
	float table_leg_width_depth = 0.02;
	float table_leg_height = 0.4;
	float table_uniform_scale = 1;
	// ----table end-------------------


	//---- fire guard start ---------------
	bool is_fire_guard = true;
	float fire_guard_radius = 1.0;
	int fire_guard_subdiv = 42;
	int num_brick_rows = 5;
	float brick_width = 0.1;
	float brick_height = 0.03;
	float brick_depth = 0.103;
	float fire_guard_x_position = 0.0;
	float fire_guard_y_position = -0.25;
	float fire_guard_z_position = 0.0;
	float fire_guard_position[3] = { fire_guard_x_position, fire_guard_y_position, fire_guard_z_position };
	float fire_guard_x_rotation = 0.0;
	float fire_guard_y_rotation = 0.0;
	float fire_guard_z_rotation = 0.0;
	float fire_guard_rotation[3] = { fire_guard_x_rotation, fire_guard_y_rotation, fire_guard_z_rotation };
	//---- fire guard end ---------------


	// ---- log pile start-------------
	bool is_log_pile = true;
	float log_pile_initial_draw = -0.01;
	int num_bottom_logs = 2;
	int log_subdiv = 12;
	float log_radius = 0.07;
	float log_length = 0.5;
	float log_x_position = 0.0;
	float log_y_position = -0.25;
	float log_z_position = 0.0;
	float log_position[3] = { log_x_position, log_y_position, log_z_position };
	float log_x_rotation = 90;
	float log_y_rotation = 0.0;
	float log_z_rotation = 0.0;
	float log_rotation[3] = { log_x_rotation, log_y_rotation, log_z_rotation };

	// ---- log pile end-------------


	//---window start-------
	bool is_window = true;
	float window_width = 1;
	float window_height = 0.7;
	float window_depth = 0.027;
	float window_x_position = 0.4;
	float window_y_position = 0.6;
	float window_z_position = -5.0;
	float window_position[3] = { window_x_position, window_y_position, window_z_position };
	float window_x_rotation = 0.0;
	float window_y_rotation = 0.0;
	float window_z_rotation = 0.0;
	float window_rotation[3] = { window_x_rotation, window_y_rotation, window_z_rotation };
	float window_x_rotation_2 = 0.0;
	float window_y_rotation_2 = 0.0;
	float window_z_rotation_2 = 0.0;
	float window_rotation_2[3] = { window_x_rotation_2, window_y_rotation_2, window_z_rotation_2 };
	float window_outer_trim_width = 0.073;
	float window_outer_trim_depth = 0.035;
	float window_inner_trim_width = 0.035;
	float window_inner_trim_depth = 0.03;
	float window_uniform_scale = 1.0;
	//---- window end-------------



	//------- draw parent objects start--------------------------
	basic_model m_model;
	
	cgra::gl_mesh mesh_object;
	basic_model* bm_object_ptr;
	vector<basic_model*> m_all_objects;

	// log pile
	basic_model* bm_log_pile_object_ptr;
	vector<basic_model*> m_log_pile_objects;

	// fire guard
	basic_model* bm_fire_guard_object_ptr;
	vector<basic_model*> m_fire_guard_objects;

	// tree
	basic_model* bm_tree_object_ptr;
	vector<basic_model*> m_tree_objects;

	// chair
	basic_model* bm_chair_object_ptr;
	vector<basic_model*> m_chair_objects;

	// table
	basic_model* bm_table_object_ptr;
	vector<basic_model*> m_table_objects;


	// window
	basic_model* bm_window_object_ptr;
	vector<basic_model*> m_window_objects;
	//---------draw parent objects end----------------------



	//------ Ryan's parameters end --------------------

	// last input
	bool m_leftMouseDown = false;
	glm::vec2 m_mousePosition;

	// drawing flags
	bool m_show_axis = false;
	bool m_show_grid = false;
	bool m_showWireframe = false;

	// geometry
	basic_model m_groundPlane;



public:


	struct boid {
		int id = -1;
		int team = 0;
		basic_model model;
		vec3 pos = vec3(0);
		vec3 vel = vec3(0);
		vec3 acc = vec3(0);
		bool leader = false;
		int leaderTime = 150;

	};

	int boidNum = 15;
	int teamNum = 3;
	vector<boid> boids;
	void spawnBoids(int numBoids);
	float searchR = 0.7f;
	float avoidanceWeight = 1.0;
	float cohesionWeight = 3.0;
	float alignmentWeight = 3.0;

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


	//-------- ryan's methods start ---------------------------
	cgra::gl_mesh sphere_oblong(int theta_subdiv, int phi_subdiv, float radius, float phi_radius_increment, float theta_radius_increment, float x_position, float y_position, float z_position, float x_rotation, float y_rotation, float z_rotation);
	cgra::gl_mesh torus(int theta_subdiv, int phi_subdiv, float theta_radius, float phi_radius,
		float x_position, float y_position, float z_position);
	cgra::gl_mesh cylinder(int subdiv, float top_radius, float bottom_radius, float height, bool is_cylinder_fill_top, bool is_cylinder_fill_bottom, float x_position, float y_position, float z_position, float x_rotation, float y_rotation, float z_rotation);
	cgra::gl_mesh cylinder_tree(int subdiv, float top_radius, float bottom_radius, float height, bool is_cylinder_fill_top, bool is_cylinder_fill_bottom, float x_position, float y_position, float z_position);
	cgra::gl_mesh cylinder_branch(int subdiv, float top_radius, float bottom_radius, float height, bool is_cylinder_fill_top, bool is_cylinder_fill_bottom, float x_position, float y_position, float z_position);
	cgra::gl_mesh rectangular_prism(float r_p_width, float r_p_height, float r_p_depth,
		float r_p_x_position, float r_p_y_position, float r_p_z_position,
		float r_p_x_rotation, float r_p_y_rotation, float r_p_z_rotation,
		float r_p_x_rotation_2, float r_p_y_rotation_2, float r_p_z_rotation_2,
		float uniform_scale);

	void tree(int tree_subdiv, float tree_height, float tree_top_radius, float tree_bottom_radius,   // tree parameters
		int branch_subdiv, int num_branch_rows, float branches_start, float branches_end, float branch_top_radius, float branch_bottom_radius,  // branch parameters
		float uniform_scale, float tree_x_position, float tree_y_position, float tree_z_position,
		float tree_x_rotation, float tree_y_rotation, float tree_z_rotation);

	void table(float table_top_width, float table_top_height, float table_top_depth,
		float table_x_position, float table_y_position, float table_z_position,
		float table_x_rotation, float table_y_rotation, float table_z_rotation,
		float table_x_rotation_2, float table_y_rotation_2, float table_z_rotation_2,
		float table_leg_width_depth, float table_leg_height, float table_uniform_scale);

	void chair(float seat_width, float seat_height, float seat_depth,
		float seat_x_position, float seat_y_position, float seat_z_position,
		float seat_x_rotation, float seat_y_rotation, float seat_z_rotation,
		float seat_x_rotation_2, float seat_y_rotation_2, float seat_z_rotation_2,
		float leg_width_depth, float leg_height,
		float b_support_height, float b_support_rest_height, float chair_uniform_scale);
	
	void fire_guard(float fire_guard_radius, int fire_guard_subdiv, int num_brick_rows,
		float brick_width, float brick_height, float brick_depth,
		float fire_guard_x_position, float fire_guard_y_position, float fire_guard_z_position,
		float fire_guard_x_rotation, float fire_guard_y_rotation, float fire_guard_z_rotation);

	void log_pile(int num_bottom_logs, int log_subdiv, float log_radius, float log_length,
		float log_x_position, float log_y_position, float log_z_position,
		float log_x_rotation, float log_y_rotation, float log_z_rotation);



	void window(float window_width, float window_height, float window_depth,
		float window_x_position, float window_y_position, float window_z_position,
		float window_x_rotation, float window_y_rotation, float window_z_rotation,
		float window_x_rotation_2, float window_y_rotation_2, float window_z_rotation_2,
		float outer_trim_width, float outer_trim_depth,
		float inner_trim_width, float inner_trim_depth);




	//-------- ryan's methods ends ---------------------------
};