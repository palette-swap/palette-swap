#pragma once
#include "rapidjson/rapidjson.h"

namespace MapUtility {
static constexpr uint8_t num_rooms = 18;
static constexpr uint8_t num_levels = 3;

// number of tutorial levels, in other word, number of
// pre-defined levels, this will help us keep track of
// procedural generated levels, so we could increase difficulty
// based on levels
static constexpr uint8_t num_tutorial_levels = 1;
} // namespace MapUtility
