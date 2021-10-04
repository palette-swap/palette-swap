#include "map_generator_system.hpp"
#include "predefined_room.hpp"

// TODO: we want this eventually be procedural generated
void MapGeneratorSystem::generateLevels()
{
	levels.emplace_back(map_layout_1);
	currentLevel = 0;
}

const MapGeneratorSystem::mapping& MapGeneratorSystem::currentMap()
{
	assert(currentLevel != -1);
	return levels[currentLevel];
}

bool MapGeneratorSystem::walkable(uvec2 pos)
{
	uint8_t room_index = currentMap().at(pos.y / 10).at(pos.x / 10);
	uint8_t tile_index = room_layouts.at(room_index).at(pos.y % 10).at(pos.x % 10);
	return WalkableTiles.find(tile_index) != WalkableTiles.end();
}
