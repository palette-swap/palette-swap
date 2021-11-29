#pragma once

#include "common.hpp"
#include "map_utility.hpp"

#include <map>
#include <random>
#include <set>

#include "rapidjson/document.h"

// Static helper class for procedural generation
class MapGenerator {
public:
	// delete constructors to make the class static
	MapGenerator() = delete;
	~MapGenerator() = default;
	MapGenerator(const MapGenerator&) = delete;
	MapGenerator(const MapGenerator&&) = delete;
	MapGenerator operator=(const MapGenerator&) = delete;
	MapGenerator& operator=(MapGenerator&&) = delete;

private:
	// room types for procedural generation
	enum class RoomType : uint8_t {
		Entrance, // starting room
		Exit, // ending room
		Side, // side room
		Critical, // rooms that the player will definitely come across
	};

	// if we want our generation parameters to be more independent,
	// we will need different engines
	struct RoomGenerationEngines {
		// for path generation, block generation
		std::default_random_engine general_eng;
		// traps generation
		std::default_random_engine traps_eng;
		// enemy generation
		std::default_random_engine enemy_random_eng_red;
		std::default_random_engine enemy_random_eng_blue;

		// currently using a single seed seems to be enough, if we want more
		// variety, we could use multiple seeds
		explicit RoomGenerationEngines(unsigned int seed)
		{
			general_eng.seed(seed);
			traps_eng.seed(seed);
			enemy_random_eng_red.seed(seed);
			// use a different seed for the other dimension
			enemy_random_eng_blue.seed(seed);
		}
	};

	// represent a room position when we generate the path in a level
	struct PathNode {
		int position; // position calculated by row * map_size + col
		bool on_main_path;
		RoomType room_type;
		PathNode * parent = nullptr;
		// neighbours on all directions, bi-directional
		std::set<PathNode *> children;
		PathNode(int position, bool on_main_path, RoomType room_type) : position(position), on_main_path(on_main_path), room_type(room_type) {}
		std::set<Direction> get_room_open_directions() const;
	};

	// generate a path from node 
	static bool generate_path_from_node(PathNode * curr_room, int path_length, std::set<int> & visited_rooms, std::default_random_engine & random_eng, RoomType room_type);

	// iterate through the generated path(graph), and generate each room layout and enemies
	// In-order traversal will be used, during the iteration, we will naturally have an order, this
	// will enable us to have dependent rooms(e.g. locked door in room 5 while key is available in room 2)
	static void traverse_path_and_generate_rooms(PathNode * starting_node,
												 MapUtility::LevelGenConf & level_gen_conf,
												 MapUtility::LevelConfiguration & level_conf,
												 rapidjson::Document & level_snap_shot,
												 RoomGenerationEngines & room_rand_eng);

	// generate a room that has paths to all open sides, tile layout will be influenced by the level generation conf
	// the open sides will be gauranteed to have at least 2 walkable tiles at the middle(row 4,5 or col 4,5)
	// we also take in random engines so each room won't be identical
	static MapUtility::RoomLayout generate_room(const std::set<Direction>& open_directions,
												RoomType room_type,
												MapUtility::LevelGenConf level_gen_conf,
												RoomGenerationEngines random_engs,
												bool is_debugging);

	// generate enemies in a room and store in the level snapshot, we need room position on map so we can obtain enemy's
	// global map position
	static void generate_enemies(MapUtility::LevelGenConf level_gen_conf,
								 const MapUtility::RoomLayout& room_layout,
								 int room_position_on_map,
								 rapidjson::Document& level_snap_shot,
								 std::default_random_engine& enemies_random_eng,
								 std::default_random_engine& general_random_eng);

public:
	// Generate a level from given level generation conf, if is_debugging is set to true, generated level will contain
	// debug information return the generated level configuration
	static MapUtility::LevelConfiguration generate_level(MapUtility::LevelGenConf level_gen_conf, bool is_debugging);
};