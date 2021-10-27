#pragma once

#include "common.hpp"
#include "map_utility.hpp"

#include <array>

// Manages and store the generated maps
class MapGeneratorSystem {
public:
	// This mapping can be used for both map and room, as they have same size(10)
	using Mapping = std::array<std::array<MapUtility::RoomType, MapUtility::room_size>, MapUtility::room_size>;
private:
	int current_level = -1;

	// the (procedural) generated levels, each level contains a full map(max 10*10 rooms),
	// index of the vector will be the map id
	std::vector<Mapping> levels;

	// get the tile texture id, of the position on a map
	MapUtility::TileId get_tile_id_from_map_pos(uvec2 pos) const;

	// Defines the room csv file paths
	const std::array<std::string, MapUtility::num_room> room_paths
		= { rooms_layout_path("full_opening_room.csv"), // 0
		    rooms_layout_path("bot_closing_room.csv"), // 1
			rooms_layout_path("left_closing_room.csv"), // 2
			rooms_layout_path("right_closing_room.csv"), // 3
			rooms_layout_path("top_closing_room.csv"), // 4
			rooms_layout_path("room_with_stair.csv"), // 5
		};

	// Defines the structure used for these rooms, which is a 2-d array
	// The index will be the room ID that uniquely identifies a room
	// The order follows the ones defined in room_paths
	// Room layout should be populated in the constructor before doing
	// any operations on map system
	std::array<Mapping, MapUtility::num_room> room_layouts;

	// Load the predefined rooms from csv files to room_layouts,
	// list of files to read will be from room_paths
	void load_rooms_from_csv();

	std::vector<uvec2> MapGeneratorSystem::bfs(uvec2 start_pos, uvec2 target) const;

public:
	MapGeneratorSystem();

	// Generates the levels, should be called at the beginning of the game,
	// might want to pass in a seed in the future
	void generate_levels();

	// Get the current level mapping
	const Mapping& current_map() const;

	// Check if a position is within the bounds of the current level
	bool is_on_map(uvec2 pos) const;

	// Check if a position on the map is walkable for the player
	bool walkable(uvec2 pos) const;

	// Check if a position on the map is walkable for the player and there's currently no entity in it
	bool walkable_and_free(uvec2 pos) const;

	// Check if a position on the map is a wall
	bool is_wall(uvec2 pos) const;

	// Computes the shortest path from start to the first element of end that it encounters via BFS
	// Returns the path, or an empty vector if no path was found
	std::vector<uvec2> shortest_path(uvec2 start, uvec2 target, bool use_a_star = true) const;

	MapUtility::TileId get_tile_id_from_room(MapUtility::RoomType room_type, uint8_t row, uint8_t col) const;
};
