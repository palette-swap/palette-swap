#pragma once

#include "common.hpp"
#include "map_utility.hpp"

#include <map>
#include <random>
#include <set>

// Helper class to MapGeneratorSystem
class MapGenerator {
private:
	// room_layouts that contains generated rooms,
	// each element is a 10*10 array that defines each tile textures in the 10*10 room
	using RoomLayout = std::array<uint32_t, MapUtility::room_size * MapUtility::room_size>;
	std::vector<RoomLayout> room_layouts;

	// generated levels
	std::vector<MapUtility::Grid> generated_levels;

	std::vector<std::string> level_snap_shots;

    // Per-Level generation configuation, used as the metadata for generating a level,
	// this is meant to be editable and serializable,
	struct LevelGenConf {
		// The seed for generating pseudo random numbers, this is a constant so we generate the
		// same map every time.
		unsigned int generation_seed = 10;
		// The number of rooms from start to end on a certain level
		unsigned int level_path_length = 6;
		
		// room-specific properties 
		// determines how uniformly the rooms are influenced by the following properties
		float room_differences = 1.0f;
		// decides how many wall tiles are spawned in a room
		double room_density = 0.2;
		// how many side paths we have, 1 indicates there a side path besides each room
		double room_side_path_density = 0.2;
		// how complex the path in a room is
		double room_path_complexity = 0.5;

        // chances of a horizontal/vertical room is converted to a corridor
		double room_corridors = 0.5;
	};
	// the generation configuration for each level
	std::vector<LevelGenConf> level_generation_confs;
	
	std::default_random_engine random_eng;

	// generate a random path of length
	// std::vector<int> generate_random_path(int start_row, int start_col, int path_length, const LevelGenConf& level_gen_conf);

    // room types for procedural generation
	enum class RoomType : uint8_t {
		Start, // starting room
		End, // ending room
		Side, // side room
		Critical, // rooms that the player will definitely come across
	};

	// generate a room that has paths to all open sides, tile layout will be influenced by the level generation conf
	// the open sides will be gauranteed to have at least 2 walkable tiles at the middle(row 4,5 or col 4,5)
    void generate_room(const std::set<Direction> & open_sides_TODO_REWORK_THIS, unsigned int level, RoomType room_type);

public:
	const std::array<uint32_t, MapUtility::map_size * MapUtility::map_size>&
	get_generated_room_layout(MapUtility::RoomID room_id) const;

	const MapUtility::Grid& get_generated_level_layout(unsigned int level) const;

    // get the level generation configuration vector, it is non-const because it is 
	// used by the map editor
	std::vector<LevelGenConf> & get_level_generation_confs();

	const std::vector<std::string>& get_level_snap_shots() const;

    // get number of generated levels
	unsigned int get_num_generated_levels() const;
	// get number of generated rooms
	unsigned int get_num_generated_rooms() const; 

	// Generates the levels, should be called at the beginning of the game,
	// might want to pass in a seed in the future
	void generate_level(int level);

};