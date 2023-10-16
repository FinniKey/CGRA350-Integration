#include "geometry.h"
using namespace std;
using namespace cgra;
using namespace glm;

gl_mesh geometry::plane(float size) {
	MeshData meshData;

	float halfSize = size * 0.5f;


	meshData.vertices = {
	vec3(-halfSize, 0.0f, -halfSize),
	vec3(halfSize, 0.0f, -halfSize),
	vec3(halfSize, 0.0f, halfSize),
	vec3(-halfSize, 0.0f, halfSize)
	};

	meshData.normals = {
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	};

	meshData.uvs = {
		vec2(0.0f, 0.0f),
		vec2(1.0f, 0.0f),
		vec2(1.0f, 1.0f),
		vec2(0.0f, 1.0f)
	};

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