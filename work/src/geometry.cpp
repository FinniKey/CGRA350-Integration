#include "geometry.h"

using namespace std;
using namespace cgra;
using namespace glm;

gl_mesh geometry::plane(float size, const glm::vec3& origin) {
	MeshData meshData;

	// Define the vertices for a single plane (two triangles).
	meshData.vertices = {
		glm::vec3(-size * 0.5f, 0.0f, -size * 0.5f) + origin,
		glm::vec3(size * 0.5f, 0.0f, -size * 0.5f) + origin,
		glm::vec3(size * 0.5f, 0.0f, size * 0.5f) + origin,
		glm::vec3(-size * 0.5f, 0.0f, size * 0.5f) + origin
	};

	// Define the normals for a single plane (all point upwards).
	meshData.normals = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	};

	// Define the UV coordinates for a single plane.
	meshData.uvs = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(1.0f, 1.0f),
		glm::vec2(0.0f, 1.0f)
	};

	// Define the indices for the two triangles forming the plane.
	meshData.indices = { 0, 1, 2, 0, 2, 3 };

	mesh_builder mb;

	unsigned int indexOffset = 0;

	for (unsigned int index : meshData.indices) {
		mesh_vertex mv;

		mv.pos = meshData.vertices[index];
		mv.norm = meshData.normals[index];
		mv.uv = meshData.uvs[index];
		mb.push_vertex(mv);

		mb.push_index(indexOffset);
		indexOffset++;
	}

	gl_mesh mesh = mb.build();

	return mesh;
}
