#include "components.hpp"
#include "render_system.hpp" // for gl_has_errors

#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb_image/stb_image.h"

// stlib
#include <iostream>
#include <sstream>

std::unordered_map<EnemyType, char*> enemy_type_to_string({ { EnemyType::Slime, "Slime" },
															{ EnemyType::Raven, "Raven" },
															{ EnemyType::LivingArmor, "LivingArmor" },
															{ EnemyType::TreeAnt, "TreeAnt" } });

// Very, VERY simple OBJ loader from https://github.com/opengl-tutorials/ogl tutorial 7
// (modified to also read vertex color and omit uv and normals)
bool Mesh::load_from_obj_file(const std::string& obj_path,
						   std::vector<ColoredVertex>& out_vertices,
						   std::vector<uint16_t>& out_vertex_indices,
						   vec2& out_size)
{
	// disable warnings about fscanf and fopen on Windows
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

	printf("Loading OBJ file %s...\n", obj_path.c_str());
	// Note, normal and UV indices are currently not used
	std::vector<uint16_t> out_uv_indices, out_normal_indices;
	std::vector<glm::vec2> out_uvs;
	std::vector<glm::vec3> out_normals;

	FILE* file = fopen(obj_path.c_str(), "r");
	if (file == nullptr) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (true) {
		std::array<char, 128> line_header = {};
		// read the first word of the line
		int res = fscanf(file, "%s", line_header.data());
		if (res == EOF) {
			break; // EOF = End Of File. Quit the loop.
		}

		if (strcmp((char*)line_header.data(), "v") == 0) {
			ColoredVertex vertex;
			fscanf(file, "%f %f %f %f %f %f\n", &vertex.position.x, &vertex.position.y, &vertex.position.z,
				   &vertex.color.x, &vertex.color.y, &vertex.color.z);
			out_vertices.push_back(vertex);
		} else if (strcmp((char*)line_header.data(), "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you
						  // want to use TGA or BMP loaders.
			out_uvs.push_back(uv);
		} else if (strcmp((char*)line_header.data(), "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			out_normals.push_back(normal);
		} else if (strcmp((char*)line_header.data(), "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			uvec3 vertex_index, normal_index; // , uvIndex[3]
			// int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertex_index[0], &uvIndex[0],
			// &normal_index[0], &vertex_index[1], &uvIndex[1], &normal_index[1], &vertex_index[2], &uvIndex[2],
			// &normal_index[2]);
			int matches = fscanf(file, "%d//%d %d//%d %d//%d\n", &vertex_index[0], &normal_index[0], &vertex_index[1],
								 &normal_index[1], &vertex_index[2], &normal_index[2]);
			if (matches != 6) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			// -1 since .obj starts counting at 1 and OpenGL starts at 0
			out_vertex_indices.push_back((uint16_t)vertex_index[0] - 1);
			out_vertex_indices.push_back((uint16_t)vertex_index[1] - 1);
			out_vertex_indices.push_back((uint16_t)vertex_index[2] - 1);
			// out_uv_indices.push_back(uvIndex[0] - 1);
			// out_uv_indices.push_back(uvIndex[1] - 1);
			// out_uv_indices.push_back(uvIndex[2] - 1);
			out_normal_indices.push_back((uint16_t)normal_index[0] - 1);
			out_normal_indices.push_back((uint16_t)normal_index[1] - 1);
			out_normal_indices.push_back((uint16_t)normal_index[2] - 1);
		} else {
			// Probably a comment, eat up the rest of the line
			std::array<char, 1000> stupid_buffer = {};
			fgets(stupid_buffer.data(), 1000, file);
		}
	}
	fclose(file);

	// Compute bounds of the mesh
	vec3 max_position = { -99999, -99999, -99999 };
	vec3 min_position = { 99999, 99999, 99999 };
	for (ColoredVertex& pos : out_vertices) {
		max_position = glm::max(max_position, pos.position);
		min_position = glm::min(min_position, pos.position);
	}
	min_position.z = 0; // don't scale z direction
	max_position.z = 1;
	vec3 size3d = max_position - min_position;
	out_size = size3d;

	// Normalize mesh to range -0.5 ... 0.5
	for (ColoredVertex& pos : out_vertices) {
		pos.position = ((pos.position - min_position) / size3d) - vec3(0.5f, 0.5f, 0.f);
	}

	return true;
}
