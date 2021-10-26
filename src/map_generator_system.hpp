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
	/////////////////////////////////////////////
	// Helper functions to retrieve file paths
	static std::string rooms_layout_path(const std::string& name)
	{
		return data_path() + "/rooms/" + std::string(name);
	};
	static std::string levels_path(const std::string& name)
	{
		return data_path() + "/levels/" + std::string(name);
	};
	static std::string room_rotations_path(const std::string& name)
	{
		return data_path() + "/level_room_rotations/" + std::string(name);
	};

	////////////////////////////////
	/// Actual file paths
	const std::array<std::string, MapUtility::num_rooms> room_paths = {
		rooms_layout_path("room_full_opening.csv"), // 0
		rooms_layout_path("room_bottom_closing.csv"), // 1
		rooms_layout_path("room_left_closing.csv"), // 2
		rooms_layout_path("room_right_closing.csv"), // 3
		rooms_layout_path("room_top_closing.csv"), // 4
		rooms_layout_path("room_next_level.csv"), // 5
		rooms_layout_path("room_big_top_left.csv"), // 6
		rooms_layout_path("room_big_top_right.csv"), // 7
		rooms_layout_path("room_big_side_top.csv"), // 8
		rooms_layout_path("room_void.csv"), // 9
		rooms_layout_path("room_next_level_help.csv"), // 10
	};
	const std::array<std::string, MapUtility::num_levels> level_paths
		= { levels_path("help_level.csv"), levels_path("level_one.csv"), levels_path("level_two.csv") };
	const std::array<std::string, MapUtility::num_levels> room_rotation_paths = { room_rotations_path("help_level.csv"),
																		  room_rotations_path("level_one.csv"),
																		  room_rotations_path("level_two.csv") };

	int current_level = -1;

	// the (procedural) generated levels, each level contains a full map(max 10*10 rooms),
	// index of the vector will be the map id
	std::vector<Mapping> levels;

	std::vector<std::array<std::array<Direction, MapUtility::map_size>, MapUtility::map_size>> level_room_rotations;

	// get the tile texture id, of the position on a map
	MapUtility::TileId get_tile_id_from_map_pos(uvec2 pos) const;
	

	// Defines the structure used for these rooms, which is a 2-d array
	// The index will be the room ID that uniquely identifies a room
	// The order follows the ones defined in room_paths
	// Room layout should be populated in the constructor before doing
	// any operations on map system
	std::array<Mapping, MapUtility::num_rooms> room_layouts;

	// Load the predefined rooms from csv files to room_layouts,
	// list of files to read will be from room_paths
	void load_rooms_from_csv();

	std::vector<uvec2> MapGeneratorSystem::bfs(uvec2 start_pos, uvec2 target) const;
	void load_levels_from_csv();

	// helper to convert an integer to Direction, as we save integers in csv files
	// 0 - UP(no rotation)
	// 1 - RIGHT
	// 2 - DOWN
	// 3 - LEFT
	Direction integer_to_direction(int direction);

public:
	MapGeneratorSystem();

	// Generates the levels, should be called at the beginning of the game,
	// might want to pass in a seed in the future
	void generate_levels();

	// Get the current level mapping
	const Mapping& current_map() const;

	const std::array<std::array<Direction, MapUtility::map_size>, MapUtility::map_size> & current_rooms_rotation() const;

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

	MapUtility::TileId get_tile_id_from_room(MapUtility::RoomType room_type, uint8_t row, uint8_t col, Direction rotation) const;
};
