// internal
#include "render_system.hpp"

#include <array>
#include <fstream>

#include "../ext/stb_image/stb_image.h"

// stlib
#include <iostream>
#include <sstream>

// World initialization
bool RenderSystem::init(
	int width, int height, GLFWwindow* window_arg, std::shared_ptr<MapGeneratorSystem> map)
{
	screen_size = { width, height };
	screen_size_capped = { width, height };
	map_generator = std::move(map);
	this->window = window_arg;
	this->debugging = debugging;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // vsync

	// Load OpenGL function pointers
	const int is_fine = gl3w_init();
	assert(is_fine == 0);

	// Create a frame buffer
	frame_buffer = 0;
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();

	// For some high DPI displays (ex. Retina Display on Macbooks)
	// https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value
	screen_scale = static_cast<float>(window_width_px) / static_cast<float>(width) * window_default_scale;

	// ASK(Camilo): Setup error callback. This can not be done in mac os, so do not enable
	// it unless you are on Linux or Windows. You will need to change the window creation
	// code to use OpenGL 4.3 (not suported on mac) and add additional .h and .cpp
	// glDebugMessageCallback((GLDEBUGPROC)errorCallback, nullptr);

	// We are not really using VAO's but without at least one bound we will crash in
	// some systems.
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	gl_has_errors();

	init_screen_texture();
	initialize_gl_textures();
	initialize_gl_effects();
	initialize_gl_geometry_buffers();

	return true;
}

RenderSystem::RenderSystem(Debug& debugging)
	: debugging(debugging) {};

void RenderSystem::initialize_gl_textures()
{
	glGenTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());

	for (uint i = 0; i < texture_paths.size(); i++) {
		const std::string& path = texture_paths.at(i);
		ivec2& dimensions = texture_dimensions.at(i);

		stbi_uc* data = nullptr;
		data = stbi_load(path.c_str(), &dimensions.x, &dimensions.y, nullptr, 4);

		if (data == nullptr) {
			const std::string message = "Could not load the file " + path + ".";
			fprintf(stderr, "%s", message.c_str());
			assert(false);
		}
		glBindTexture(GL_TEXTURE_2D, texture_gl_handles.at(i));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		// This might be useful for some pictures, will leave it here for reference
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		gl_has_errors();
		stbi_image_free(data);
	}

	TTF_Init();
	gl_has_errors();
}

void RenderSystem::initialize_gl_effects()
{
	for (uint i = 0; i < effect_paths.size(); i++) {
		const std::string vertex_shader_name = effect_paths.at(i) + ".vs.glsl";
		const std::string fragment_shader_name = effect_paths.at(i) + ".fs.glsl";

		bool is_valid = load_effect_from_file(vertex_shader_name, fragment_shader_name, effects.at(i));
		assert(is_valid && (GLuint)effects.at(i) != 0);
	}
}

// One could merge the following two functions as a template function...
template <class T> void RenderSystem::bind_vbo_and_ibo(uint gid, std::vector<T> vertices, std::vector<uint16_t> indices)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers.at((uint)gid));
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	gl_has_errors();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers.at((uint)gid));
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, static_cast<int>(sizeof(indices[0]) * indices.size()), indices.data(), GL_STATIC_DRAW);
	gl_has_errors();
}

void RenderSystem::initialize_gl_meshes()
{
	for (const auto& path : mesh_paths) {
		// Initialize meshes
		Mesh& mesh = meshes.at((int)path.first);
		std::string name = path.second;
		Mesh::load_from_obj_file(name, mesh.vertices, mesh.vertex_indices, mesh.original_size);

		bind_vbo_and_ibo((uint)path.first, mesh.vertices, mesh.vertex_indices);
	}
}

void RenderSystem::initialize_gl_geometry_buffers()
{
	// Vertex Buffer creation.
	glGenBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
	// Index Buffer creation.
	glGenBuffers((GLsizei)index_buffers.size(), index_buffers.data());

	// Index and Vertex buffer data initialization.
	initialize_gl_meshes();

	//////////////////////////
	// Initialize sprite
	// The position corresponds to the center of the texture.
	// TODO keep sprites that have not been adjusted here for now
	// Eventually consolidate into a single animatedSpriteVertex
	std::vector<TexturedVertex> textured_vertices(4);
	textured_vertices[0].position = { -1.f / 2, +1.f / 2, 0.f };
	textured_vertices[1].position = { +1.f / 2, +1.f / 2, 0.f };
	textured_vertices[2].position = { +1.f / 2, -1.f / 2, 0.f };
	textured_vertices[3].position = { -1.f / 2, -1.f / 2, 0.f };
	textured_vertices[0].texcoord = { 0.f, 1.f };
	textured_vertices[1].texcoord = { 1.f, 1.f };
	textured_vertices[2].texcoord = { 1.f, 0.f };
	textured_vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> textured_indices = { 0, 3, 1, 1, 3, 2 };
	bind_vbo_and_ibo((uint)GEOMETRY_BUFFER_ID::SPRITE, textured_vertices, textured_indices);

	////////////////////////////
	// Initialize TileMap
	const int total_vertices = MapUtility::room_size * MapUtility::room_size * 2 * 3;
	std::vector<int> tilemap_vertices(total_vertices);
	std::vector<uint16_t> tilemap_indices(total_vertices);
	for (int i = 0; i < total_vertices; i++) {
		tilemap_vertices[i] = i;
		tilemap_indices[i] = static_cast<uint16_t>(i);
		bind_vbo_and_ibo((uint)GEOMETRY_BUFFER_ID::ROOM, tilemap_vertices, tilemap_indices);
	}

	//////////////////////////
	// TODO: Consolidate all animated sprites (quads) into a single type
	// Initialize ENEMY sprites
	// The position corresponds to the center of the texture.
	std::vector<SmallSpriteVertex> enemy_vertices(4);
	enemy_vertices[0].position = { -1.f / 2, +1.f / 2, 0.f };
	enemy_vertices[1].position = { +1.f / 2, +1.f / 2, 0.f };
	enemy_vertices[2].position = { +1.f / 2, -1.f / 2, 0.f };
	enemy_vertices[3].position = { -1.f / 2, -1.f / 2, 0.f };
	enemy_vertices[0].texcoord = { 0, sprite_size/spritesheet_height};
	enemy_vertices[1].texcoord = { sprite_size / spritesheet_width, sprite_size / spritesheet_height };
	enemy_vertices[2].texcoord = { sprite_size / spritesheet_width, 0 };
	enemy_vertices[3].texcoord = { 0, 0 };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> enemy_indices = { 0, 3, 1, 1, 3, 2 };
	bind_vbo_and_ibo((uint)GEOMETRY_BUFFER_ID::SMALL_SPRITE, enemy_vertices, enemy_indices);

	//////////////////////////////////
	// Initialize health bars
	std::vector<ColoredVertex> health_vertices;
	std::vector<uint16_t> health_indices;

	constexpr float depth = 0.8f;

	constexpr vec3 white = { 1, 1, 1 };
	constexpr vec3 red = { 0.8, 0.1, 0.1 };
	constexpr vec3 black = { 0, 0, 0 };

	// Corner points
	health_vertices = {
		{ { 0, -.5, depth }, white },
		{ { 0, .5, depth }, white },
		{ { 1, .5, depth }, white },
		{ { 1, -.5, depth }, white },
		{ { 0, -.5, depth }, black },
		{ { 0, .5, depth }, black },
		{ { 1, .5, depth }, black },
		{ { 1, -.5, depth }, black },
	};

	// Two triangles
	health_indices = { 4, 5, 7, 5, 6, 7, 0, 1, 3, 1, 2, 3 };

	Mesh& health = meshes.at((int)GEOMETRY_BUFFER_ID::HEALTH);
	health.vertices = health_vertices;
	health.vertex_indices = health_indices;
	bind_vbo_and_ibo((uint)GEOMETRY_BUFFER_ID::HEALTH, health_vertices, health_indices);

	//////////////////////////////////
	// Initialize regular line
	std::vector<ColoredVertex> line_vertices;
	std::vector<uint16_t> line_indices;

	// Corner points
	line_vertices = {
		{ { -0.5, -0.5, depth }, white },
		{ { -0.5, 0.5, depth }, white },
		{ { 0.5, 0.5, depth }, white },
		{ { 0.5, -0.5, depth }, white },
	};

	// Two triangles
	line_indices = { 0, 1, 3, 1, 2, 3 };

	Mesh& line = meshes.at((int)GEOMETRY_BUFFER_ID::LINE);
	line.vertices = line_vertices;
	line.vertex_indices = line_indices;
	bind_vbo_and_ibo((uint)GEOMETRY_BUFFER_ID::LINE, line_vertices, line_indices);

	//////////////////////////////////
	// Initialize debug line
	std::vector<ColoredVertex> debug_line_vertices;

	// Corner points
	line_vertices = {
		{ { -0.5, -0.5, depth }, red },
		{ { -0.5, 0.5, depth }, red },
		{ { 0.5, 0.5, depth }, red },
		{ { 0.5, -0.5, depth }, red },
	};

	Mesh& debug_line = meshes.at((int)GEOMETRY_BUFFER_ID::DEBUG_LINE);
	debug_line.vertices = debug_line_vertices;
	debug_line.vertex_indices = line_indices;
	bind_vbo_and_ibo((uint)GEOMETRY_BUFFER_ID::DEBUG_LINE, debug_line_vertices, line_indices);

	///////////////////////////////////////////////////////
	// Initialize screen triangle (yes, triangle, not quad; its more efficient).
	std::vector<vec3> screen_vertices(3);
	screen_vertices[0] = { -1, -6, 0.f };
	screen_vertices[1] = { 6, -1, 0.f };
	screen_vertices[2] = { -1, 6, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> screen_indices = { 0, 1, 2 };
	bind_vbo_and_ibo((uint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE, screen_vertices, screen_indices);
}

RenderSystem::~RenderSystem()
{
	try {
		// Don't need to free gl resources since they last for as long as the program,
		// but it's polite to clean after yourself.
		glDeleteBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
		glDeleteBuffers((GLsizei)index_buffers.size(), index_buffers.data());
		glDeleteTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());
		glDeleteTextures(1, &off_screen_render_buffer_color);
		glDeleteRenderbuffers(1, &off_screen_render_buffer_depth);
		gl_has_errors();

		for (const auto effect : effects) {
			glDeleteProgram(effect);
		}
		// delete allocated resources
		glDeleteFramebuffers(1, &frame_buffer);
		gl_has_errors();

		// Delete text-related resources
		for (auto& text_data : text_buffers) {
			glDeleteTextures(1, &text_data.second.texture);
		}
		for (auto& font : fonts) {
			TTF_CloseFont(font.second);
		}

		// remove all entities created by the render system
		auto view = registry.view<RenderRequest>();
		registry.destroy(view.begin(), view.end());
	} catch (...) {
		/* Destructors really shouldn't throw anything */
	}
}

// Initialize the screen texture from a standard sprite
bool RenderSystem::init_screen_texture()
{
	registry.emplace<ScreenState>(screen_state_entity);

	glGenTextures(1, &off_screen_render_buffer_color);
	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width_px, window_height_px, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();

	glGenRenderbuffers(1, &off_screen_render_buffer_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, off_screen_render_buffer_depth);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, off_screen_render_buffer_color, 0);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width_px, window_height_px);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, off_screen_render_buffer_depth);
	gl_has_errors();

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	return true;
}

bool gl_compile_shader(GLuint shader)
{
	glCompileShader(shader);
	gl_has_errors();
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		GLint log_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> log(log_len);
		glGetShaderInfoLog(shader, log_len, &log_len, log.data());
		glDeleteShader(shader);

		gl_has_errors();

		fprintf(stderr, "GLSL: %s", log.data());
		return false;
	}

	return true;
}

bool load_effect_from_file(const std::string& vs_path, const std::string& fs_path, GLuint& out_program)
{
	// Opening files
	std::ifstream vs_is(vs_path);
	std::ifstream fs_is(fs_path);
	if (!vs_is.good() || !fs_is.good()) {
		fprintf(stderr, "Failed to load shader files %s, %s", vs_path.c_str(), fs_path.c_str());
		assert(false);
		return false;
	}

	// Reading sources
	std::stringstream vs_ss, fs_ss;
	vs_ss << vs_is.rdbuf();
	fs_ss << fs_is.rdbuf();
	std::string vs_str = vs_ss.str();
	std::string fs_str = fs_ss.str();
	const char* vs_src = vs_str.c_str();
	const char* fs_src = fs_str.c_str();
	auto vs_len = (GLsizei)vs_str.size();
	auto fs_len = (GLsizei)fs_str.size();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vs_src, &vs_len);
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fs_src, &fs_len);
	gl_has_errors();

	// Compiling
	if (!gl_compile_shader(vertex)) {
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}
	if (!gl_compile_shader(fragment)) {
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}

	// Linking
	out_program = glCreateProgram();
	glAttachShader(out_program, vertex);
	glAttachShader(out_program, fragment);
	glLinkProgram(out_program);
	gl_has_errors();

	{
		GLint is_linked = GL_FALSE;
		glGetProgramiv(out_program, GL_LINK_STATUS, &is_linked);
		if (is_linked == GL_FALSE) {
			GLint log_len = 0;
			glGetProgramiv(out_program, GL_INFO_LOG_LENGTH, &log_len);
			std::vector<char> log(log_len);
			glGetProgramInfoLog(out_program, log_len, &log_len, log.data());
			gl_has_errors();

			fprintf(stderr, "Link error: %s", log.data());
			assert(false);
			return false;
		}
	}

	// No need to carry this around. Keeping these objects is only useful if we recycle
	// the same shaders over and over, which we don't, so no need and this is simpler.
	glDetachShader(out_program, vertex);
	glDetachShader(out_program, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	gl_has_errors();

	return true;
}
