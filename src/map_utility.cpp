#include "map_utility.hpp"

MapUtility::MapAreaIterator::MapAreaIterator(uint min_x, uint max_x, uvec2 current_pos)
	: min_x(min_x)
	, max_x(max_x)
	, current_pos(current_pos)
{
}

MapUtility::MapArea::MapArea(const MapPosition& map_pos, const MapSize& map_size)
	: map_pos(map_pos)
	, map_size(map_size)
{
}
