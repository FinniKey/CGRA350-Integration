#pragma once
#include <vector>

#include "particle_system.hpp"
#include "cgra/cgra_shader.hpp"
using namespace std;
using namespace cgra;

class smoke_system
{
	float height = 25;
	float width = 25;
	float depth = 25;

	int subdivisions = 25;

	vec3 offset = vec3(0, 0, 0); //Offset the grid by something.


	vector<vector<vector<int>>> grid_data;
	Particle_System p_system;


public:
	smoke_system(float h = 5, float w = 5, float d = 5, int s = 5)
	{
		height = h;
		width = w;
		depth = d;
		subdivisions = s;

		
		initialize_grid();
		
		shader_builder particle_sb;
		particle_sb.set_shader(GL_VERTEX_SHADER, CGRA_SRCDIR + std::string("//res//shaders//particle_vert.glsl"));
		particle_sb.set_shader(GL_FRAGMENT_SHADER, CGRA_SRCDIR + std::string("//res//shaders//particle_frag.glsl"));
		GLuint particle_shader = particle_sb.build();

		p_system = Particle_System(particle_shader);
		p_system.set_particle_limit(25);
		

	}


	void initialize_grid()
	{
	

		grid_data.clear();

		for (int x = 0; x < subdivisions; x++) {

			vector<vector<int>> newX;
			grid_data.push_back(newX);
			for (int y = 0; y < subdivisions; y++)
			{
				vector<int> newY;
				grid_data[x].push_back(newY);

				for (int z = 0; z < subdivisions; z++)
				{
					grid_data[x][y].push_back(1);
				}
			}

		}
	}

	vec3 locationToGridIndex(vec3 point)
	{
		int xIndex = (int)(point.x / (width/subdivisions));
		int yIndex = (int)(point.y / (height / subdivisions));
		int zIndex = (int)(point.z / (depth / subdivisions));

		
		//If it's outside the grid, ignore it.
		if (xIndex < 0 || xIndex >=width
			|| yIndex < 0 || yIndex >=height
			|| zIndex < 0 || zIndex >=depth)
		{
			return vec3(-1, -1, -1);
		}

		return vec3(xIndex, yIndex, zIndex);

	}

	void draw(const glm::mat4& view, const glm::mat4 proj, float delta_time)
	{
		p_system.draw(view, proj, delta_time);
	}

	void update()
	{
		initialize_grid();

		//For all the particles, update how many are in the grid
		
		for (int i = 0; i < p_system.get_particles().size(); i++)
		{
			Particle_System::Particle p = p_system.get_particles().at(i);
			vec3 grid_location = locationToGridIndex(p.Position);
			if (grid_location == vec3(-1, -1, -1)) //If outside the grid, ignore it.
			{
				continue;
			}
			
			//Add it to the grid data
			grid_data[static_cast<int>(grid_location.x)][static_cast<int>(grid_location.y)][static_cast<int>(grid_location.z)]++;
		}

	}


	vec3 get_surrounding_elements(int x, int y, int z)
	{
		std::vector<int> above, below, left, right, front, behind;



		vec3 force = vec3(0.0f);

		// Negate the force vector to get the opposite direction
		return -force;


	}




	void print_data()
	{
		

		for (int x = 0; x < subdivisions; x++) {

	
			for (int y = 0; y < subdivisions; y++)
			{
				

				for (int z = 0; z < subdivisions; z++)
				{
					if (grid_data[x][y][z] != 0)
					{
						cout << x << "," << y << "," << z << ": " << grid_data[x][y][z] << "\n";

					}
				}
			}

		}

	}





};