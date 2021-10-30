#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"

#include "map_generator_system.hpp"

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
	
	// TODO: change all asset paths to reference corresponding folder instead
	// for each corresponding enemy/player type, such that it grabs general folder path
	// and then uses path to grab sprites, stats, and behaviour
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, texture_count> texture_paths = {
			textures_path("./01/Player Spritesheet.png"),
			textures_path("./Slime/Slime Spritesheet.png"),
			textures_path("./Living Armor/Living Armor Spritesheet.png"),
			textures_path("./Treeant/Treeant Spritesheet.png"),
			textures_path("./Raven/Raven Spritesheet.png"),
			textures_path("./Wraith/Wraith Spritesheet.png"),
			textures_path("./Drake/Drake Spritesheet.png"),
			textures_path("cannon_ball.png"),
			textures_path("tile_set.png"), };

	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = { shader_path("line"),
																 shader_path("enemy"), 
																 shader_path("player"),
																 shader_path("health_bar"),
																 shader_path("textured"),
																 shader_path("water"),
																 shader_path("tilemap") };

	// TODO: move these constants into animation system most likely, need to finalize
	// hierachy between animation and render system
	// Filler for experimental spritesheet dimensions (3 rows and 4 columns)
	// Will likely update to a standard square (regardless of rows or columns of actual sprites used)
	static constexpr float spritesheet_width = 4.f;
	static constexpr float spritesheet_height = 3.f;
	// Should be kept in tune with tile size, but this remains to be seen
	static constexpr float sprite_size = 1.f;
	static constexpr float player_spritesheet_width = 8.f;
	static constexpr float player_spritesheet_height = 4.f;

	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	std::array<Mesh, geometry_count> meshes;

	// initialize all predefined rooms, based on roomtype
	void initialize_room_vertices(MapUtility::RoomType roomType);

	std::shared_ptr<MapGeneratorSystem> map_generator;
public:
	// Initialize the window
	bool init(int width, int height, GLFWwindow* window, std::shared_ptr<MapGeneratorSystem> map);

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
	vec2 get_top_left();
	void scale_on_scroll(float offset);

	void on_resize(int width, int height);

	float screen_scale; // Screen to pixel coordinates scale factor (for apple
					// retina display?)

private:
	// Helper to get position transform
	Transform get_transform(Entity entity);

	// Internal drawing functions for each entity type
	void draw_textured_mesh(Entity entity, const mat3& projection);
	void draw_healthbar(Entity entity, const Stats& stats, const mat3& projection);
	void draw_to_screen();
	void update_camera_position(MapPosition& camera_map_pos,
								const vec2& player_pos,
								const vec2& buffer_top_left,
								const vec2& buffer_down_right);

	// Window handle
	GLFWwindow* window;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	Entity screen_state_entity;
	ivec2 screen_size = { window_width_px, window_height_px };
};

bool load_effect_from_file(const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
