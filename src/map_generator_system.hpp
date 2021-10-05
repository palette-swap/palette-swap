#pragma once

#include "common.hpp"

// Manages and store the generated maps
class MapGeneratorSystem {
private:
	int current_level = -1;
	using Mapping = std::array<std::array<RoomType, room_size>, room_size>;

	// the (procedural) generated levels, each level contains a full map(max 10*10 rooms)
	std::vector<Mapping> levels;

public:

	// Generates the levels, should be called at the beginning of the game,
	// might want to pass in a seed in the future
	void generate_levels();

	// Get the current level mapping
	const Mapping& current_map() const;

	// Check if a position is within the bounds of the current level
	bool is_on_map(uvec2 pos) const;

	// Check if a position on the map is walkable for the player
	bool walkable(uvec2 pos) const;

	// Computes the shortest path from start to the first element of end that it encounters via BFS
	// Returns the path, or an empty vector if no path was found
	std::vector<uvec2> shortest_path(uvec2 start, std::vector<uvec2> end) const;
};
