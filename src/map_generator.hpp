#pragma once

#include "common.hpp"

// Manages and store the generated maps
class MapGenerator {
private:
	int currentLevel = -1;

public:

	using mapping = std::array<std::array<RoomType, ROOM_SIZE>, ROOM_SIZE>;

	// the (procedural) generated levels, each level contains a full map(max 10*10 rooms)
	std::vector<mapping> levels;

	// Generates the levels, should be called at the beginning of the game,
	// might want to pass in a seed in the future
	void generateLevels();

	// Get the current level mapping
	const mapping& currentMap();

	// Check if a position on the map is walkable for the player
	bool walkable(uvec2 pos);
};
