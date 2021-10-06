#pragma once

#include "common.hpp"

// Manages and store the generated maps
class MapGeneratorSystem {
public:
	using Mapping = std::array<std::array<RoomType, room_size>, room_size>;
private:
	int current_level = -1;

	// the (procedural) generated levels, each level contains a full map(max 10*10 rooms)
	std::vector<Mapping> levels;

public:

	// Generates the levels, should be called at the beginning of the game,
	// might want to pass in a seed in the future
	void generate_levels();

	// Get the current level mapping
	const Mapping& current_map();

	// Check if a position on the map is walkable for the player
	bool walkable(uvec2 pos);
};
