#pragma once

#include "common.hpp"
#include "components.hpp"
#include "map_utility.hpp"
class TurnSystem;

#include <array>
#include <set>

namespace MapUtility {
//////////////////////////////////////////
// Defines different types of tiles
const std::set<uint8_t>& walkable_tiles();
const std::set<uint8_t>& wall_tiles();
const uint8_t tile_next_level = 14;
const uint8_t tile_last_level = 20;
} // namespace MapUtility

// Manages and store the generated maps
class MapGeneratorSystem {
public:
	// This mapping can be used for both map and room, as they have same size(10)
	using Mapping = std::array<std::array<MapUtility::RoomType, MapUtility::room_size>, MapUtility::room_size>;
private:
	std::shared_ptr<TurnSystem> turns;
	/////////////////////////////////////////////
	// Helper functions to retrieve file paths
	static std::string rooms_layout_path(const std::string& name)
	{
		return data_path() + "/rooms/" + std::string(name);
	};
	static std::string levels_path(const std::string& name)
	{
		return data_path() + "/level_rooms/" + std::string(name);
	};
	static std::string room_rotations_path(const std::string& name)
	{
		return data_path() + "/level_room_rotations/" + std::string(name);
	};
	static std::string level_configurations_path(const std::string& name)
	{
		return data_path() + "/level_configurations/" + std::string(name);
	}

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
		rooms_layout_path("room_last_level.csv"), // 11
		rooms_layout_path("room_bot_opening.csv"), // 12
		rooms_layout_path("room_left_opening.csv"), // 13
		rooms_layout_path("room_right_opening.csv"), // 14
		rooms_layout_path("room_top_opening.csv"), // 15
		rooms_layout_path("room_vertical.csv"), // 16
		rooms_layout_path("room_horizontal.csv"), // 17
	};
	const std::array<std::string, MapUtility::num_levels> level_paths
		= { levels_path("help_level.csv"), levels_path("level_one.csv"), levels_path("level_two.csv") };
	const std::array<std::string, MapUtility::num_levels> room_rotation_paths
		= { room_rotations_path("help_level.csv"),
			room_rotations_path("level_one.csv"),
			room_rotations_path("level_two.csv") };
	const std::array<std::string, MapUtility::num_levels> level_configuration_paths = {
		level_configurations_path("help_level.json"),
		level_configurations_path("level_one.json"),
		level_configurations_path("level_two.json"),
	};

	/////////////////////////////////////
	// Loaders
	void load_rooms_from_csv(); 
	void load_levels_from_csv(); // load both level and rooms rotations
	void load_level_configurations();
	void load_enemies_from_file();


	///////////////////////////////////////////////////////////
	// Data structures thats saving the information loaded
	//
	// room_layouts define the predefined rooms, index is the room id, that follows the order
	// in room_paths, each element is a 10*10 array that defines each tile textures in the 10*10 room
	// Note: we are saving uint32_t for tile ids, but tile_id is actually 8 bit, this is because opengl
	// shaders only have 32 bit ints. However, number of tile textures are restricted in 8 bit integer(
	// since the walkable/wall tiles set are uint8_t and our tile atlas only support 64 tiles),
	// so it should always be safe to convert from uint32_t to uin8_t
	std::array<std::array<uint32_t, MapUtility::map_size * MapUtility::map_size>, MapUtility::num_rooms> room_layouts;
	// the (procedural) generated levels, each level contains a full map(max 10*10 rooms),
	// index of the vector will be the map id
	std::vector<Mapping> levels;
	std::vector<std::array<std::array<Direction, MapUtility::map_size>, MapUtility::map_size>> level_room_rotations;
	// Involving all dynamic components on a map, stored in json format
	std::vector<std::string> level_snap_shots;


	// End of file loaders
	/////////////////////////////

	int current_level = -1;
	
	// Entity for the help picture
	Entity help_picture = entt::null;
	void create_picture();

	// get the tile texture id, of the position on a map
	MapUtility::TileID get_tile_id_from_map_pos(uvec2 pos) const;

	std::vector<uvec2> bfs(uvec2 start_pos, uvec2 target) const;

	// helper to convert an integer to Direction, as we save integers in csv files
	// 0 - UP(no rotation)
	// 1 - RIGHT
	// 2 - DOWN
	// 3 - LEFT
	Direction integer_to_direction(int direction);

	// Take current level snapshot
	void snapshot_level();

	// Clear current level
	void clear_level() const;

	// Load level specified, if snap shot exists, load from it,
	// Otherwise load from statically defined files(TODO: use procedural generation instead)
	void load_level(int level);

	// Create the current map
	void create_map(int level) const;
	// Create a room
	void create_room(vec2 position, MapUtility::RoomType roomType, float angle) const;

public:
	explicit MapGeneratorSystem(std::shared_ptr<TurnSystem> turns);

	// Generates the levels, should be called at the beginning of the game,
	// might want to pass in a seed in the future
	void generate_levels();

	// Get the current level mapping
	const Mapping& current_map() const;

	const std::array<std::array<Direction, MapUtility::map_size>, MapUtility::map_size>&
	current_rooms_rotation() const;

	// Check if a position is within the bounds of the current level
	bool is_on_map(uvec2 pos) const;

	// Check if a position on the map is walkable for the player
	bool walkable(uvec2 pos) const;

	// Check if a position on the map is walkable for the player and there's currently no entity in it
	bool walkable_and_free(uvec2 pos, bool check_active_color = true) const;

	// Check if a position on the map is a wall
	bool is_wall(uvec2 pos) const;

	// Computes the shortest path from start to the first element of end that it encounters via BFS
	// Returns the path, or an empty vector if no path was found
	std::vector<uvec2> shortest_path(uvec2 start, uvec2 target, bool use_a_star = true) const;

	MapUtility::TileID
	get_tile_id_from_room(MapUtility::RoomType room_type, uint8_t row, uint8_t col, Direction rotation) const;

	bool is_next_level_tile(uvec2 pos) const;
	bool is_last_level_tile(uvec2 pos) const;

	bool is_last_level() const;

	// Save current level and load the next level
	void load_next_level();

	// Save current level and load the last level
	void load_last_level();

	// Load the first level
	void load_initial_level();

	// Get player initial position on current level
	uvec2 get_player_start_position() const;

	// Get player last position on current level
	uvec2 get_player_end_position() const;

	// Get the 10*10 layout array for a room, mainly used by rendering
	const std::array<uint32_t, MapUtility::map_size * MapUtility::map_size>&
	get_room_layout(MapUtility::RoomType type) const;
};
