#pragma once

#include "common.hpp"
#include "components.hpp"
#include "map_generator.hpp"
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
private:
	std::shared_ptr<TurnSystem> turns;
	/////////////////////////////////////////////
	// Helper functions to retrieve file paths
	static std::string predefined_rooms_path(const std::string& name)
	{
		return data_path() + "/rooms/" + std::string(name);
	};
	static std::string levels_path(const std::string& name)
	{
		return data_path() + "/level_rooms/" + std::string(name);
	};
	static std::string level_configurations_path(const std::string& name)
	{
		return data_path() + "/level_configurations/" + std::string(name);
	}
	static std::string level_generation_conf_path(const std::string& name)
	{
		return data_path() + "/level_generation_conf/" + std::string(name);
	}

	////////////////////////////////
	/// Actual file paths
	const std::array<std::string, MapUtility::num_predefined_rooms> predefined_room_paths = {
		predefined_rooms_path("room_big_top_left.csv"), // 0
		predefined_rooms_path("room_big_top_right.csv"), // 1
		predefined_rooms_path("room_big_bot_left.csv"), // 2
		predefined_rooms_path("room_big_bot_right.csv"), // 3
		predefined_rooms_path("room_big_side_top.csv"), // 4
		predefined_rooms_path("room_big_side_bot.csv"), // 5
		predefined_rooms_path("room_void.csv"), // 6
		predefined_rooms_path("room_big_side_bot_next_level.csv"), // 7
	};
	const std::array<std::string, MapUtility::num_predefined_levels> predefined_level_paths
		= { levels_path("help_level.csv") };
	const std::array<std::string, MapUtility::num_predefined_levels> level_configuration_paths
		= { level_configurations_path("help_level.json") };

	// Load predefined levels, including predefined rooms and level configurations,
	// Note: this is expected to be called before precedural generation, as it resize the
	// level configuration vector to number of predefined levels
	void load_predefined_level_configurations();
	// load generated levels to level configurations, so it's ready to be used
	void load_generated_level_configurations();

	// Note: the number of generation files won't be fixed, so we use the level number as each file name
	// e.g. conf for first generated level will be 0.json, second will be 1.json...
	void load_level_generation_confs();

	// saves configurations for each level
	std::vector<MapUtility::LevelConfiguration> level_configurations;

	// generation configurations for each procedurally generated levels, indexed by level
	// the actual level is calculated as index + num_predefined_levels
	std::vector<MapUtility::LevelGenConf> level_generation_confs;

	// Getters for each specific level configurations
	const MapUtility::MapLayout& get_level_layout(int level) const;
	const std::string& get_level_snap_shot(int level) const;
	const std::vector<MapUtility::RoomLayout>& get_level_room_layouts(int level) const;

	int current_level;

	// get the tile texture id, of the position on the current level
	MapUtility::TileID get_tile_id_from_map_pos(uvec2 pos) const;

	std::vector<uvec2> bfs(uvec2 start_pos, uvec2 target) const;

	// Take current level snapshot
	void snapshot_level();

	// Clear current level
	void clear_level() const;

	// Load level specified
	void load_level(int level);

	// Create the current map
	void create_map(int level) const;
	// Create a room
	void create_room(vec2 position, MapUtility::RoomID room_id, int level) const;

	// Entity for the help picture
	Entity help_picture = entt::null;
	void create_picture();

	// these two backups are for recovering from map editing mode, can also be potentially
	// used for save and load
	std::vector<MapUtility::LevelConfiguration> level_configurations_backup;
	int current_level_backup;

public:
	explicit MapGeneratorSystem(std::shared_ptr<TurnSystem> turns);
	void init();

	// Get the current level mapping
	const MapUtility::MapLayout& current_map() const;

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

	MapUtility::TileID get_tile_id_from_room(int level, MapUtility::RoomID room_id, uint8_t row, uint8_t col) const;

	// TODO: probably shouldn't expose these, we should have public step_on_tile function, and do the processing internally 
	bool is_next_level_tile(uvec2 pos) const;
	bool is_last_level_tile(uvec2 pos) const;
	bool is_trap_tile(uvec2 pos) const;

	bool is_last_level() const;

	// Save current level and load the next level
	void load_next_level();

	// Save current level and load the last level
	void load_last_level();

	// Load the first level
	void load_initial_level();

	// Get the 10*10 layout array for a room, mainly used by rendering
	const std::array<uint32_t, MapUtility::map_size * MapUtility::map_size>&
	get_room_layout(int level, MapUtility::RoomID room_id) const;

	/////////////////////////////////////////////////
	// Map Editor
	void start_editing_level();
	void stop_editing_level();
	void save_level_generation_confs();
	void edit_next_level();
	void edit_previous_level();
	void regenerate_map();
	void increment_seed();
	void decrement_seed();
	void increment_path_length();
	void decrement_path_length();
	void increase_room_density();
	void decrease_room_density();
	void increase_side_rooms();
	void decrease_side_rooms();
	void increase_room_path_complexity();
	void decrease_room_path_complexity();
	void increase_room_traps_density();
	void decrease_room_traps_density();
	void increase_room_smoothness();
	void decrease_room_smoothness();
};
