#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem {
	/**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
	std::array<GLuint, texture_count> texture_gl_handles;
	std::array<ivec2, texture_count> texture_dimensions;

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	const std::vector<std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths = {
		{ GEOMETRY_BUFFER_ID::SALMON, mesh_path("salmon.obj") }
		// specify meshes of other assets here
	};

	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, texture_count> texture_paths = {
			textures_path("Paladin_A01.png"),
			textures_path("Slime.png"),
			textures_path("Slime alert.png"),
			textures_path("Slime flinched.png"),
			textures_path("Arrow.png"),
			textures_path("tile_set.png"), };

	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths
		= { shader_path("line"), shader_path("textured"), shader_path("water"), shader_path("tilemap") };

	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	std::array<Mesh, geometry_count> meshes;

	// initialize all predefined rooms, based on roomtype
	void initialize_room_vertices(RoomType roomType);

public:
	// Initialize the window
	bool init(int width, int height, GLFWwindow* window);

	// Modified first argument to gid, which doesn't change behavior and is reasonable,
	// it also helps the room geometry hack to work...
	template <class T> void bind_vbo_and_ibo(uint gid, std::vector<T> vertices, std::vector<uint16_t> indices);

	void initialize_gl_textures();

	void initialize_gl_effects();

	void initialize_gl_meshes();
	Mesh& get_mesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

	void initialize_gl_geometry_buffers();
	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the water
	// shader
	bool init_screen_texture();

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// rule of five
	RenderSystem() = default;
	RenderSystem(const RenderSystem&) = delete; // copy constructor
	RenderSystem& operator=(const RenderSystem&) = delete; // copy assignment
	RenderSystem(RenderSystem&&) = delete; // move constructor
	RenderSystem& operator=(RenderSystem&&) = delete; // move assignment

	// Draw all entities
	void draw();

	mat3 create_projection_matrix();
	void scale_on_scroll(float offset);

	float screen_scale; // Screen to pixel coordinates scale factor (for apple
					// retina display?)

private:
	// Internal drawing functions for each entity type
	void draw_textured_mesh(Entity entity, const mat3& projection);
	void draw_to_screen();

	// Window handle
	GLFWwindow* window;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	Entity screen_state_entity;
};

bool load_effect_from_file(const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
