﻿#pragma once

// stlib
#include <array>
#include <fstream> // stdout, stderr..
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>

// This must be included after gl3w.h
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/ext/vector_int2.hpp> // ivec2
#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/mat3x3.hpp> // mat3
#include <glm/vec2.hpp> // vec2
#include <glm/vec3.hpp> // vec3
using namespace glm;

#include <entt/entity/registry.hpp>

#include "soloud.h"

#include <filesystem>

// Simple utility functions to avoid mistyping directory name
// audio_path("audio.ogg") -> data/audio/audio.ogg
// Get defintion of PROJECT_SOURCE_DIR from:
#include "../ext/project_path.hpp"
inline std::string project_source_dir()
{
	std::string source_dir = std::filesystem::current_path().make_preferred().string() + "/";
	std::replace(source_dir.begin(), source_dir.end(), '\\', '/');
	return source_dir;
}
inline std::string data_path() { return project_source_dir() + "data"; };
inline std::string shader_path(const std::string& name)
{
	return project_source_dir() + "/shaders/" + name;
};
inline std::string textures_path(const std::string& name) { return data_path() + "/textures/" + std::string(name); };
inline std::string audio_path(const std::string& name) { return data_path() + "/audio/" + std::string(name); };
inline std::string music_path(const std::string_view& name) { return data_path() + "/audio/music/" + std::string(name); };
inline std::string fonts_path(const std::string& name) { return data_path() + "/fonts/" + std::string(name); };
inline std::string mesh_path(const std::string& name) { return data_path() + "/meshes/" + std::string(name); };
inline std::string json_schema_path(const std::string& name) { return data_path() + "/schemata/" + std::string(name); };

// The 'Transform' component handles transformations passed to the Vertex shader
// (similar to the gl Immediate mode equivalent, e.g., glTranslate()...)
// We recomment making all components non-copyable by derving from ComponentNonCopyable
struct Transform {
	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f } }; // start with the identity
	void scale(vec2 scale);
	void rotate(float radians);
	void translate(vec2 offset);
};

bool gl_has_errors();

// Window sizes
static constexpr int window_width_px = 1920;
static constexpr int window_height_px = 1080;
static constexpr ivec2 window_default_size = vec2(window_width_px, window_height_px);
static constexpr float window_default_scale = 0.5;

namespace AnimationUtility {
// Defines default colors for enemies of red/blue while active
static constexpr vec3 default_enemy_red = { 4, 1, 1 };
static constexpr vec3 default_enemy_blue = { 1, 1, 4 };

// Used to test out various inactive enemy displays
static constexpr vec4 inactive_invisible = { 1, 1, 1, 0 };
} // namespace AnimationUtility

namespace MapUtility {
// Each tile is 32x32 pixels
static constexpr float tile_size = 32.f;
// Each room is 10x10 tiles
static constexpr int room_size = 10;
// Each map is 10x10 rooms
static constexpr int map_size = 10;
using RoomID = uint8_t;
using TileID = uint8_t;

static constexpr uvec2 map_top_left = { 0, 0 };
static constexpr uvec2 map_down_right
	= { MapUtility::room_size * MapUtility::map_size - 1, MapUtility::room_size* MapUtility::map_size - 1 };

// Calculates virtual position of top left corner of map given screen and mapsystem constants
// Currently used for actual_position to render_position convertion
static constexpr vec2 top_left_corner = vec2((window_width_px - tile_size * room_size * map_size) / 2,
											 (window_height_px - tile_size * room_size * map_size) / 2);

// Some ASCII art to explain... It's basically coordinate system conversion
// TODO: This might need to be in the camera system after it's added
/**
* (0,0)            3200*3200
  ┌─────────────────────────────────────┐
  │                                     │
  │                                     │
  │                                     │
  │                                     │
  │                                     │
  │        1920*1080                    │
  │        ┌────────────────────┐       │
  │        │                    │       │
  │        │                    │       │
  │        │                    │       │
  │        │                    │       │
  │        │                    │       │
  │        └────────────────────┘       │
  │                                     │
  │                                     │
  │                                     │
  │                                     │
  │                                     │
  │                                     │
  └─────────────────────────────────────┘
										(3200, 3200)
										(1920, 1080)
										(99,99)
*/
inline vec2 map_position_to_world_position(uvec2 map_pos)
{
	return vec2(map_pos) * tile_size + top_left_corner + vec2(tile_size / 2, tile_size / 2);
}

// Calculates which square an entity is currently overlapping
inline uvec2 world_position_to_map_position(vec2 screen_pos)
{
	return uvec2(((screen_pos.x) - top_left_corner.x) / tile_size, ((screen_pos.y) - top_left_corner.y) / tile_size);
}

inline std::vector<uvec2> get_surrounding_tiles(uvec2 center, uint radius = 1u)
{
	std::vector<uvec2> tiles((radius * 2 + 1) * (radius * 2 + 1));
	for (uint i = 0; i <= radius * 2; i++) {
		for (uint j = 0; j <= radius * 2; j++) {
			if (center.x + i < radius || center.y + j < radius) {
				continue;
			}
			tiles.emplace_back(center.x + i - radius, center.y + j - radius);
		}
	}

	return tiles;
}

inline uint8_t get_room_index(uvec2 tile)
{
	uvec2 room_pos = tile / static_cast<uint>(room_size);
	return static_cast<uint8_t>(room_pos.y * map_size + room_pos.x);
}

inline std::pair<uvec2, uvec2> get_room_area(uint8_t room) {
	uvec2 top_left = uvec2(room % 10, room / 10) * static_cast<uint>(room_size);
	uvec2 bottom_right = uvec2((room) % 10 + 1, room / 10 + 1) * static_cast<uint>(room_size) - uvec2(1);
	return std::make_pair<>(top_left, bottom_right);
}

} // namespace MapUtility

// Represent four directions, that could have many uses, e.g. moving player
enum class Direction : uint8_t {
	Left,
	Up,
	Right,
	Down,
  Undefined
};

namespace CameraUtility {
// the size of camera, divide the whole window into camera_grid_size ^ 2 smaller grids
static constexpr uint camera_grid_size = 3;
// area buffer: the offset between the buffer area and the edge of camera
// buffer_offset * 2 < camera_grid_size
static constexpr uint camera_buffer_offset = 1;

// calculate buffer position based on the size and offset
inline std::pair<vec2, vec2> get_buffer_positions(vec2 camera_pos, float width, float height)
{
	assert(camera_buffer_offset * 2 < camera_grid_size);
	vec2 offset = vec2(width, height) * (1.f - 2.f * camera_buffer_offset / camera_grid_size) / 2.0f;

	vec2 buffer_top_left_pos = camera_pos - offset;
	vec2 buffer_down_right_pos = camera_pos + offset;

	return std::make_pair(buffer_top_left_pos, buffer_down_right_pos);
}
} // namespace CameraUtility

// Explanation for camera
/**
*

 top-left corner: camera.pos (initialize as player starting point)
 The distance between buffer area and the edge of camera is
	(width | height / size) * offset
 width*height (dynamically determined by window resize)
 camera_grid_size = 7, buffer_offset = 1
  ┌─────────────────────────────────────┐
  │                                     │
  │	  1    :         5           :   1  │
  │--------┌────────────────────┐-------│
  │        │                    │       │
  │        │     buffer         │       │
  │        │     area           │       │
  │        │                    │       │
  │        │                    │       │
  │        └────────────────────┘       │
  │                                     │
  └─────────────────────────────────────┘
  The vertical ratio is the same

  5*5/7*7 of the whole window is buffer
  only update the camera position when 2 requirments are fufilled:
	  1. The player move out of the buffer area
	  2. The camera edge is not at the edge of the whole map
	  (if camera located at (0, 0) and player is at the left of buffer, camera won't update since it's already at the
edge of map)

*/

extern entt::registry registry; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

using Entity = entt::entity;
