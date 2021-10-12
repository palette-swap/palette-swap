#pragma once

#include <array>

/**
 * This file defines different static room types,
 * Each room is a 10 * 10 2d array, array element is an uint8_t,
 * this number is the tile id that uniquely identifies a specific tile texture(see
 * tile_textures in components.hpp)
 * 
 * Each room has an id that uniquely identifies the room, see room_layouts in common.hpp
*/

/**
* How to add new tile textures
* 1. add to TEXTURE_ASSET_ID in components.hpp
* 2. add to texture_paths in render_system.hpp
* 3. add to tile_textures in components.hpp
* 4. Update MapUtility::num_tile_textures in common.hpp and add to tile_textures[] array in components.hpp.
*    This is for assigning an unique id for each texture starting from 0
* 5. Update size of tile_textures in shaders\tilemap.fs.glsl
* Then the tile can be referenced with id in the predefined room arrays.
* First two steps are common for all textures, last 3 steps are map specific.
*/

/**
* How to add a new room
* Define the 2d array in this file, top left is (0,0) and bottom right is (9,9),
* then add the new room to room_layouts, index is the room id.
* Then the room can be referenced as id in the map definition(e.g. map_layout_1)
*/

// 0
static constexpr std::array<std::array<uint8_t, 10>,10> room_left_right_1 = {
    1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,1,2,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,
};

// 1
static constexpr std::array<std::array<uint8_t, 10>, 10> room_top_down_1 = {
    1,1,1,0,0,0,0,1,1,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,1,1,0,0,0,1,
    1,0,0,0,1,1,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    1,1,1,0,0,0,0,1,1,1,
};

// 2
static constexpr std::array<std::array<uint8_t, 10>, 10> room_all_direction_1 = {
    1,1,1,0,0,0,0,1,1,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,1,1,0,0,0,0,
    0,0,0,1,2,1,0,0,0,0,
    0,0,0,0,1,0,0,0,0,0,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    1,1,1,0,0,0,0,1,1,1,
};

// Statically defined rooms, associate room layout to room id
static constexpr std::array<std::array<std::array<uint8_t, 10>, 10>, 3> room_layouts = {
	room_left_right_1,
	room_top_down_1,
	room_all_direction_1,
};

// We also define some static maps, this should be eventually replaced with procedural generation
static constexpr std::array<std::array<uint8_t, 10>, 10> map_layout_1 = {
    0,0,0,0,0,0,0,0,0,0,
    0,2,2,0,0,0,0,2,2,0,
    0,2,2,0,0,0,0,2,2,0,
    0,0,1,2,2,2,2,2,0,0,
    0,0,1,2,0,0,0,2,0,0,
    0,0,2,1,0,2,0,1,0,0,
    0,0,2,2,0,2,0,2,0,0,
    0,1,1,0,0,0,0,1,1,0,
    0,1,1,0,0,0,0,1,2,0,
    0,2,2,2,2,2,2,2,2,0,
};