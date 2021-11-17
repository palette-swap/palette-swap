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
	MapGenerator(const MapGenerator&) = delete;
	MapGenerator(const MapGenerator&&) = delete;
	MapGenerator operator=(const MapGenerator&) = delete;

private:
	// room types for procedural generation
	enum class RoomType : uint8_t {
		Start, // starting room
		End, // ending room
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
		// enemies generation
		std::default_random_engine enemies_eng;
		// currently using a single seed seems to be enough, if we want more
		// variety, we could use multiple seeds
		RoomGenerationEngines(unsigned int seed)
		{
			general_eng.seed(seed);
			traps_eng.seed(seed);
			enemies_eng.seed(seed);
		}
	};

	// generate a room that has paths to all open sides, tile layout will be influenced by the level generation conf
	// the open sides will be gauranteed to have at least 2 walkable tiles at the middle(row 4,5 or col 4,5)
	// we also take in random engines so each room won't be identical
	static MapUtility::RoomLayout generate_room(const std::set<Direction>& open_directions,
												RoomType room_type,
												MapUtility::LevelGenConf level_gen_conf,
												RoomGenerationEngines random_engs,
												bool is_debugging);

public:
	// Generate a level from given level generation conf, if is_debugging is set to true, generated level will contain
	// debug information return the generated level configuration
	static MapUtility::LevelConfiguration generate_level(MapUtility::LevelGenConf level_gen_conf, bool is_debugging);
};