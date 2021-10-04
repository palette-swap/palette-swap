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

// 0
static constexpr std::array<std::array<uint8_t, 10>,10> room_left_right_1 = {
    1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,1,1,0,0,0,0,
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
    0,0,0,0,1,1,0,0,0,0,
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