#pragma once

#include <array>
#include <utility>
#define SDL_MAIN_HANDLED
#include <SDL_ttf.h>

#include "common.hpp"
#include "components.hpp"

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
	std::array<GLuint, texture_count> texture_gl_handles = {};
	std::array<ivec2, texture_count> texture_dimensions = {};

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
			textures_path("./01-Player/Player Spritesheet.png"),
			textures_path("./TrainingDummy/Dummy Spritesheet.png"),
			textures_path("./Slime/Slime Spritesheet.png"),
			textures_path("./Living Armor/Living Armor Spritesheet.png"),
			textures_path("./Treeant/Treeant Spritesheet.png"),
			textures_path("./Raven/Raven Spritesheet.png"),
			textures_path("./Wraith/Wraith Spritesheet.png"),
			textures_path("./Drake/Drake Spritesheet.png"),
			textures_path("./Mushroom/Mushroom Spritesheet.png"),
			textures_path("./Spider/Spider Spritesheet.png"),
			textures_path("./Clone/Clone Spritesheet.png"),
			textures_path("./02-Bosses/King Mush/King Mush Spritesheet.png"),
			textures_path("cannon_ball.png"),
			textures_path("/01-Player/Spell Spritesheet.png"),
			textures_path("tile_set.png"),
			textures_path("help.png"),
			textures_path("End Screen.png") };

	std::array<GLuint, effect_count> effects = {};
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = { shader_path("line"),
																 shader_path("rectangle"),
																 shader_path("enemy"), 
																 shader_path("player"),
																 shader_path("health_bar"),
																 shader_path("fancy_bar"),
																 shader_path("textured"),
																 shader_path("spell"),
																 shader_path("water"),
																 shader_path("tilemap") };

	// TODO: move these constants into animation system most likely, need to finalize
	// hierachy between animation and render system
	// Filler for experimental spritesheet dimensions (3 rows and 4 columns)
	// Will likely update to a standard square (regardless of rows or columns of actual sprites used)
	static constexpr float spritesheet_width = 8.f;
	static constexpr float spritesheet_height = 8.f;
	// Should be kept in tune with tile size, but this remains to be seen
	static constexpr float sprite_size = 1.f;
	static constexpr float player_spritesheet_width = 8.f;
	static constexpr float player_spritesheet_height = 4.f;

	// Static buffers
	std::array<GLuint, geometry_count> vertex_buffers = {};
	std::array<GLuint, geometry_count> index_buffers = {};
	std::array<Mesh, geometry_count> meshes = {};

	// Dynamic text buffers
	struct TextData {
		GLuint texture;
		int texture_width;
		int texture_height;
	};
	std::unordered_map<Text, TextData> text_buffers = {};
	std::unordered_map<unsigned int, TTF_Font*> fonts = {};

	std::shared_ptr<MapGeneratorSystem> map_generator;
public:
	// Initialize the window
	bool init(int width, int height, GLFWwindow* window, std::shared_ptr<MapGeneratorSystem> map);

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

	// Draw UI entities over top
	void draw_ui(const mat3& projection);

	mat3 create_projection_matrix();
	vec2 screen_position_to_world_position(vec2 screen_pos);

	// WorldSystem callbacks for window changes
	void scale_on_scroll(float offset);
	void on_resize(int width, int height);

	vec2 get_screen_size() { return screen_size; }

private:
	////////////////////////////////////////////////////////
	// Init helper functions
	template <class T> void bind_vbo_and_ibo(uint gid, std::vector<T> vertices, std::vector<uint16_t> indices);
	void initialize_gl_textures();
	void initialize_gl_effects();
	void initialize_gl_meshes();
	void initialize_gl_geometry_buffers();
	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the water
	// shader
	bool init_screen_texture();

	////////////////////////////////////////////////////////
	// General helper functions
	// Get world position of top left and bottom right of screen
	std::pair<vec2, vec2> get_window_bounds();
	// Get UI scale based on difference between current window size and default
	float get_ui_scale_factor() const;
	// Helper to get position transform
	Transform get_transform(Entity entity);
	// Helper to get position transform without rotation
	Transform get_transform_no_rotation(Entity entity);

	// Helper to ready to draw the Textured effect
	void prepare_for_textured(GLuint texture_id);

	// Generates raster texture of provided text
	// Returns vbo, ibo
	TextData generate_text(const Text& text);

	////////////////////////////////////////////////////////
	// Internal drawing functions for each entity type
	void draw_textured_mesh(Entity entity, const RenderRequest& render_request, const mat3& projection);
	void draw_effect(Entity entity, const EffectRenderRequest& render_request, const mat3& projection);
	void draw_ui_element(Entity entity, const UIRenderRequest& ui_render_request, const mat3& projection);
	void draw_healthbar(Transform transform, const Stats& stats, const mat3& projection, bool fancy, float ratio);
	void draw_rectangle(Entity entity, Transform transform, vec2 scale, const mat3& projection);
	void draw_text(Entity entity, const Text& text, const mat3& projection);
	void draw_line(Entity entity, const Line& line, const mat3& projection);
	void draw_map(const mat3& projection);

	void draw_triangles(const Transform& transform, const mat3& projection);
	void draw_to_screen();
	void update_camera_position(WorldPosition& camera_world_pos,
								const vec2& player_pos,
								const vec2& buffer_top_left,
								const vec2& buffer_down_right);

	// Window handle
	GLFWwindow* window = nullptr;

	// Screen texture handles
	GLuint frame_buffer = 0;
	GLuint off_screen_render_buffer_color = 0;
	GLuint off_screen_render_buffer_depth = 0;

	Entity screen_state_entity = registry.create();
	ivec2 screen_size = { window_width_px, window_height_px };
	ivec2 screen_size_capped = { window_width_px, window_height_px };
	float screen_scale = 1; // Screen to pixel coordinates scale factor (for apple
							// retina display?)
};

bool load_effect_from_file(const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
