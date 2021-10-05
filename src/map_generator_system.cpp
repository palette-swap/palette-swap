#include "map_generator_system.hpp"
#include "predefined_room.hpp"

// TODO: we want this eventually be procedural generated
void MapGeneratorSystem::generate_levels()
{
	levels.emplace_back(map_layout_1);
	current_level = 0;
}

const MapGeneratorSystem::Mapping& MapGeneratorSystem::current_map()
{
	assert(current_level != -1);
	return levels[current_level];
}

bool MapGeneratorSystem::walkable(uvec2 pos)
{
	uint8_t room_index = current_map().at(pos.y / 10).at(pos.x / 10);
	uint8_t tile_index = room_layouts.at(room_index).at(pos.y % 10).at(pos.x % 10);
	return walkable_tiles.find(tile_index) != walkable_tiles.end();
}
