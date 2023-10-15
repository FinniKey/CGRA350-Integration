#pragma once
// glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// project
#include "opengl.hpp"
#include <vector>


// glm
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

#include <iomanip>
#include <stb_image.h>
#include "cgra/cgra_shader.hpp"

using namespace glm;
using namespace std;
using namespace cgra;

class Particle_System
{
	

public:
	struct Particle {
		glm::vec3 Position = vec3(0,0,0); // The particle's position
		glm::vec3 Velocity = vec3(0.001,0.004,0.0001); // The particle's velocity
		
		glm::vec4 Color = vec4(static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 
			static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 
			static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 
			static_cast <float> (rand()) / static_cast <float> (RAND_MAX));  // The color of the particle
		float life = 0; // How long the particle has lived.

		glm::vec3 norm;
		glm::vec2 uv = vec2(1, 1);

		int frame = 0;

		float scaling = 2555.0;
	};

private:
	vector<GLuint> textures;

	vec3 Origin = vec3(0);  //Origin point of the Particle System

	float particle_lifetime = 0.95; //How long the particles last

	int particle_limit = 16002; //The maximum number of particles

	int particle_burst_amnt = 200;

	vector<Particle> particles;

	float timer = 0;
	float burst_delay = 15;


	float max_dist = 5;

	vector<tuple<float, vec4>> gradient_colors = {
		make_tuple(0.0,vec4(1,1,1,1)), make_tuple(0.25,vec4(1, 1, 1.773,1)),
		make_tuple(0.5,vec4(1, 0.653, 0.322,1.00)), make_tuple(0.75,vec4(1, 0.522, 0.361,1.00)),
		make_tuple(1.0,vec4(1, 0.522, 0.361,1.0))
	};


	std::random_device rd;


public:

	GLuint shader = 0;

	GLuint vao = 0;
	GLuint vbo = 0;


	int get_size()
	{
		return particles.size();
	}

	// Function to load a PNG image as a texture
	GLuint loadPNGTexture(const char* filePath) {
		int width, height, channels;
		unsigned char* image = stbi_load(filePath, &width, &height, &channels, STBI_rgb_alpha);

		if (!image) {
			// Handle error (could not load the image)
			return 0;
		}

		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Set texture parameters (optional)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Upload the image data to the GPU
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Free the image data (it's already uploaded to the GPU)
		stbi_image_free(image);

		return textureID;
	}

	void set_gradient(vec4 one_col, vec4 two_col, vec4 three_col, vec4 four_col, vec4 fifth_col,
		float one_period, float two_period, float three_period, float four_period, float fifth_period)
	{
		std::vector<tuple<float,vec4>> new_gradients = {
			make_tuple(one_period,one_col), make_tuple(two_period,two_col),
			make_tuple(three_period, three_col),make_tuple(four_period,four_col),make_tuple(fifth_period,fifth_col)
		};

		gradient_colors = new_gradients;
	}
	vec4 get_color(Particle p)
	{
		for (int i = 0; i < gradient_colors.size(); i++)
		{
			float period = std::get<0>(gradient_colors.at(i));
			if ((p.life / this->particle_lifetime) < period)
			{
				float last_period = (i == 0) ? 0.0f : std::get<0>(gradient_colors.at(i - 1));
				float next_period = period;

				vec4 last_color = std::get<1>(gradient_colors.at(i - 1));
				vec4 next_color = std::get<1>(gradient_colors.at(i));

				float dist = abs(glm::distance(p.Position, this->Origin));
				float t = (dist - (max_dist * last_period)) / (max_dist * (next_period - last_period));

				return glm::mix(last_color, next_color, t);
			}
		}

		// Default to white if no matching period is found
		return vec4(1.0f);
	}

	Particle_System()
	{
		shader_builder particle_sb;
		particle_sb.set_shader(GL_VERTEX_SHADER, CGRA_SRCDIR + std::string("//res//shaders//particle_vert.glsl"));
		particle_sb.set_shader(GL_FRAGMENT_SHADER, CGRA_SRCDIR + std::string("//res//shaders//particle_frag.glsl"));
		GLuint particle_shader = particle_sb.build();

		shader = particle_shader;


		textures.clear();
		std::random_device rd;

		std::mt19937 gen(rd());

		std::uniform_real_distribution<> dist(0, 2 * glm::pi<float>());
		std::uniform_real_distribution<> height(0, 1);


		for (int i = 0; i < particle_burst_amnt; i++)
		{
			Particle p;
			// Calculate a random angle
			float angle = dist(gen);

			// Calculate a random radius within the maximum distance (max_dist)
			float radius = 5 * std::sqrt(dist(gen));

			// Calculate the initial position within a circle
			p.Position = vec3(radius * std::cos(angle), height(gen), radius * std::sin(angle));

			particles.push_back(p);
		}

		for (int i = 1; i < 481; i++)
		{
			std::string formattedNumber = std::to_string(i);
			while (formattedNumber.length() < 3) {
				formattedNumber = "0" + formattedNumber;
			}

			std::string name = "fireball03-frame" + formattedNumber + ".tga";
			GLuint texture = loadPNGTexture(std::string(CGRA_SRCDIR + std::string("//res//textures//fireballs//") + name).c_str());
			textures.push_back(texture);
		}

		GLuint pngTexture = loadPNGTexture(std::string(CGRA_SRCDIR + std::string("//res//textures//fireball.png")).c_str());

		// Bind the shader program
		glUseProgram(shader);

		// Get the location of the uniform variable in your shader
		GLint textureLocation = glGetUniformLocation(shader, "fireTexture");

		// Set the texture unit (e.g., GL_TEXTURE0)
		glActiveTexture(GL_TEXTURE0);

		// Bind the texture to the active texture unit
		glBindTexture(GL_TEXTURE_2D, pngTexture);

		// Associate the uniform variable in the shader with the texture unit
		glUniform1i(textureLocation, 0);


	}

	void set_particle_limit(int limit)
	{
		this->particle_limit = limit;
	}

	Particle_System(GLuint shad)
	{
		textures.clear();
		std::random_device rd;

		std::mt19937 gen(rd());

		std::uniform_real_distribution<> dist(0, 2 * glm::pi<float>());
		std::uniform_real_distribution<> height(0,1);

		shader = shad;

		for (int i = 0; i < particle_burst_amnt; i++)
		{
			Particle p;
			// Calculate a random angle
			float angle = dist(gen);

			// Calculate a random radius within the maximum distance (max_dist)
			float radius = 5 * std::sqrt(dist(gen));

			// Calculate the initial position within a circle
			p.Position = vec3(radius * std::cos(angle), height(gen), radius * std::sin(angle));

			particles.push_back(p);
		}
		
		GLuint pngTexture = loadPNGTexture(std::string(CGRA_SRCDIR + std::string("//res//textures//fireball.png")).c_str());

		for (int i = 1; i < 481; i++)
		{
			std::string formattedNumber = std::to_string(i);
			while (formattedNumber.length() < 3) {
				formattedNumber = "0" + formattedNumber;
			}

			std::string name = "fireball03-frame" + formattedNumber + ".tga";
			GLuint texture = loadPNGTexture(std::string(CGRA_SRCDIR + std::string("//res//textures//fireballs/") + name).c_str());
			textures.push_back(texture);
		}

		// Bind the shader program
		glUseProgram(shader);

		// Get the location of the uniform variable in your shader
		GLint textureLocation = glGetUniformLocation(shader, "fireTexture");

		// Set the texture unit (e.g., GL_TEXTURE0)
		glActiveTexture(GL_TEXTURE0);

		// Bind the texture to the active texture unit
		glBindTexture(GL_TEXTURE_2D, pngTexture);

		// Associate the uniform variable in the shader with the texture unit
		glUniform1i(textureLocation, 0);

	}

	vector<Particle>& get_particles()
	{
		return this->particles;
	}

	void draw(const glm::mat4& view, const glm::mat4 proj, float delta_time = 0.025)
	{

		std::mt19937 gen(rd());
		std::uniform_real_distribution<> dist(0, 1);
		std::uniform_real_distribution<> rad(0, 2 * glm::pi<float>()); // Uniform distribution in [0, 2 * pi]
		timer++;

		
		if (timer > burst_delay && particles.size() < this->particle_limit)
		{
			for (int i = 0; i < this->particle_burst_amnt; i++) {
				Particle p;
				// Calculate a random angle
				float angle = rad(gen);

				// Calculate a random radius within the maximum distance (max_dist)
				float radius = rad(gen);

				// Calculate the initial position within a circle
				p.Position = vec3(radius * std::cos(angle), 5, radius * std::sin(angle));

				p.Velocity = vec3(0, 12, 0);
				p.life = std::max(0.0, dist(gen));

				particles.push_back(p);
			}
			timer = 0;
		}

		


		for (Particle& p: particles)
		{
			p.Velocity.y += 0.005;
			p.Position += p.Velocity * delta_time;
			p.Color = get_color(p);
			p.life+= std::max(0.0f,delta_time);

			//p.scaling -= 1 / this->particle_lifetime;
			p.frame++;
			if (p.frame >= textures.size())
			{
				p.frame = 0;
			}

			if (p.life >= this->particle_lifetime)
			{
		

				float angle = rad(gen);

				// Calculate a random radius within the maximum distance (max_dist)
				float radius = dist(gen);

				// Calculate the initial position within a circle
				p.Position = vec3(radius * std::cos(angle) * dist(gen), dist(gen), radius * std::sin(angle) * dist(gen));
				p.Velocity = vec3(0,std::max(0.0,dist(gen)*6), 0);
				p.life = dist(gen);
				p.scaling = 1;
			}
			

		
		}

		glGenVertexArrays(1, &vao); // VAO stores information about how the buffers are set up
		glGenBuffers(1, &vbo); // VBO stores the vertex data

		glUseProgram(shader);
		glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, false, value_ptr(proj));
		glUniformMatrix4fv(glGetUniformLocation(shader, "uModelViewMatrix"), 1, false, value_ptr(view));
		glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, value_ptr(vec3(0,0,1)));

		// VAO
		//
		glBindVertexArray(vao);

		// VBO (single buffer, interleaved)
		//
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		// upload ALL the vertex data in one buffer
		glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), &particles[0], GL_STATIC_DRAW);

		// this buffer will use location=0 when we use our VAO
		glEnableVertexAttribArray(0);
		// tell opengl how to treat data in location=0 - the data is treated in lots of 3 (3 floats = vec3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(offsetof(Particle, Position)));

		// do the same thing for Normals but bind it to location=1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(offsetof(Particle, norm)));

		// do the same thing for UVs but bind it to location=2 - the data is treated in lots of 2 (2 floats = vec2)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(offsetof(Particle, uv)));

		// do the same thing for colors but bind it to location=3 - the data is treated in lots of 4 (4 floats = vec4)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(offsetof(Particle, Color)));


		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(offsetof(Particle, scaling)));


	

		// Enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		// Render objects with additive blending
		// Set colors with appropriate alpha values
		glDepthMask(GL_FALSE);
		glPointSize(25);
		// Get the location of the uniform variable in your shader
		GLint textureLocation = glGetUniformLocation(shader, "fireTexture");

		// Set the texture unit (e.g., GL_TEXTURE0)
		glActiveTexture(GL_TEXTURE0);


		for (int i =0; i < particles.size(); i++)
		{ 
		
			// Bind the texture to the active texture unit
			glBindTexture(GL_TEXTURE_2D, textures.at(particles.at(i).frame));

			// Associate the uniform variable in the shader with the texture unit
			glUniform1i(textureLocation, 0);
			glDrawArrays(GL_POINTS, i, 1);

		}
		

		// Disable blending when finished
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		glDeleteBuffers(1,&vbo);
		glDeleteVertexArrays(1, &vao);

	}

};