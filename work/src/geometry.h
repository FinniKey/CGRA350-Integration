#pragma once

#include <glm/glm.hpp>
#include "opengl.hpp"
#include "cgra/cgra_mesh.hpp"

using namespace glm;
using namespace std;

struct MeshData {
	vector<vec3> vertices;
	vector<vec3> normals;
	vector<unsigned int> indices;
	vector<vec2> uvs;
};


namespace geometry {
	cgra::gl_mesh plane(float size, const glm::vec3& origin);
}