#pragma once
#include "rapidjson/rapidjson.h"

namespace MapUtility {
static constexpr uint8_t num_predefined_rooms = 18;
static constexpr uint8_t num_predefined_levels = 3;

// number of tutorial levels, in other word, number of
// pre-defined levels, this will help us keep track of
// procedural generated levels, so we could increase difficulty
// based on levels
static constexpr uint8_t num_tutorial_levels = 1;

// 10*10 grid, this can be used for both map and room layouts, as they have same size(10)
using Grid = std::array<std::array<MapUtility::RoomID, MapUtility::room_size>, MapUtility::room_size>;
} // namespace MapUtility
