#include "map_generator.hpp"
#include "predefined_room.hpp"

// TODO: we want this eventually be procedural generated
void MapGenerator::generateLevels()
{
	levels.emplace_back(map_layout_1);
	currentLevel = 0;
}

const MapGenerator::mapping& MapGenerator::currentMap()
{
	assert(currentLevel != -1);
	return levels[currentLevel];
}

bool MapGenerator::walkable(uvec2 pos)
{
	uint8_t room_index = currentMap()[pos.y / 10][pos.x / 10];
	uint8_t tile_index = (room_layouts[room_index])[pos.y % 10][pos.x % 10];
	return WalkableTiles.find(tile_index) != WalkableTiles.end();
}

MapGenerator::MapGenerator() { }

MapGenerator::~MapGenerator() { }
