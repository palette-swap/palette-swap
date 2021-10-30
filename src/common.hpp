﻿#pragma once

// stlib
#include <fstream> // stdout, stderr..
#include <string>
#include <tuple>
#include <vector>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>

// This must be included after gl3w.h
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/ext/vector_int2.hpp> // ivec2
#include <glm/mat3x3.hpp> // mat3
#include <glm/vec2.hpp> // vec2
#include <glm/vec3.hpp> // vec3
using namespace glm;

#include "tiny_ecs.hpp"

// Simple utility functions to avoid mistyping directory name
// audio_path("audio.ogg") -> data/audio/audio.ogg
// Get defintion of PROJECT_SOURCE_DIR from:
#include "../ext/project_path.hpp"
inline std::string data_path() { return std::string(PROJECT_SOURCE_DIR) + "data"; };
inline std::string shader_path(const std::string& name)
{
	return std::string(PROJECT_SOURCE_DIR) + "/shaders/" + name;
};
inline std::string textures_path(const std::string& name) { return data_path() + "/textures/" + std::string(name); };
inline std::string audio_path(const std::string& name) { return data_path() + "/audio/" + std::string(name); };
inline std::string fonts_path(const std::string& name) { return data_path() + "/fonts/" + std::string(name); };
inline std::string mesh_path(const std::string& name) { return data_path() + "/meshes/" + std::string(name); };
inline std::string rooms_layout_path(const std::string& name) { return data_path() + "/rooms/" + std::string(name); };

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

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
static constexpr float window_default_scale = 0.5;


namespace MapUtility {
    // Each tile is 32x32 pixels
    static constexpr float tile_size = 32.f;
    // Each room is 10x10 tiles
    static constexpr int room_size = 10;
    // Each map is 10x10 rooms
    static constexpr int map_size = 10;
    // RoomType is just a uint8_t
    using RoomType = uint8_t;
    using TileId = uint8_t;
    using MapId = uint8_t;

    static const std::set<uint8_t> walkable_tiles = { 0, 14 };
    static const std::set<uint8_t> wall_tiles = {
        1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 15,
        17, 18, 19, 23, 25, 26, 27, 33, 35, 41, 42, 43 };

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
		return vec2(map_pos.x * tile_size + top_left_corner.x, map_pos.y * tile_size + top_left_corner.y)
			+ vec2(tile_size / 2, tile_size / 2);
    }

    // Calculates which square an entity is currently overlapping
    inline uvec2 world_position_to_map_position(vec2 screen_pos)
    {
		return uvec2(((screen_pos.x) - top_left_corner.x) / tile_size,
					 ((screen_pos.y) - top_left_corner.y) / tile_size);
    }
}

namespace CameraUtility {
	// the size of camera, divide the whole window into 5 smaller grids
	static constexpr uint camera_grid_size = 7;
	// area buffer: the offset between the buffer area and the edge of camera
	// buffer_offset * 2 < camera_grid_size
	static constexpr uint camera_buffer_offset = 2;

	static constexpr uint map_top_left = 0;
	static constexpr uint map_down_right = 99;

	// calculate buffer position based on the size and offset
	inline std::tuple<vec2, vec2> get_buffer_positions(vec2 camera_screen_pos, float width, float height) { 
		assert(camera_buffer_offset * 2 < camera_grid_size);
		float horizontal_offset_per_grid = width / camera_grid_size;
		float vertical_offset_per_grid = height / camera_grid_size;

		vec2 buffer_top_left_pos = vec2(camera_screen_pos.x + camera_buffer_offset * horizontal_offset_per_grid, 
			camera_screen_pos.y + camera_buffer_offset * vertical_offset_per_grid);
		vec2 buffer_down_right_pos
			= vec2(camera_screen_pos.x + (camera_grid_size - camera_buffer_offset) * horizontal_offset_per_grid,
				   camera_screen_pos.y + (camera_grid_size - camera_buffer_offset) * vertical_offset_per_grid);

		return std::make_tuple(buffer_top_left_pos, buffer_down_right_pos);
	}
}


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
	  (if camera located at (0, 0) and player is at the left of buffer, camera won't update since it's already at the edge of map)
										
*/