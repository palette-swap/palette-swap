#include "map_generator_system.hpp"
#include "predefined_room.hpp"

#include <queue>
#include <unordered_map>

#include "glm/gtx/hash.hpp"

// TODO: we want this eventually be procedural generated
void MapGeneratorSystem::generate_levels()
{
	levels.emplace_back(map_layout_1);
	current_level = 0;
}

const MapGeneratorSystem::Mapping& MapGeneratorSystem::current_map() const
{
	assert(current_level != -1);
	return levels[current_level];
}

bool MapGeneratorSystem::is_on_map(uvec2 pos) const
{
	return pos.y / room_size < current_map().size() && pos.x / room_size < current_map().at(0).size();
}

bool MapGeneratorSystem::walkable(uvec2 pos) const
{
	if (!is_on_map(pos)) {
		return false;
	}

	return walkable_tiles.find(get_tile_id(pos)) != walkable_tiles.end();
}

bool MapGeneratorSystem::is_wall(uvec2 pos) const
{
	if (!is_on_map(pos)) {
		return false;
	}

	return wall_tiles.find(get_tile_id(pos)) != wall_tiles.end();
}

// See https://en.wikipedia.org/wiki/Breadth-first_search for algorithm reference
std::vector<uvec2> MapGeneratorSystem::shortest_path(uvec2 start_pos, uvec2 target) const
{
	std::queue<uvec2> frontier;
	// Presence in parent will also be used to track visited
	std::unordered_map<uvec2, uvec2> parent;
	frontier.push(start_pos);
	parent.emplace(start_pos, start_pos);
	while (!frontier.empty()) {
		uvec2 curr = frontier.front();

		// Check if curr is an accepting state
		if (curr == target) { 
			// Now generate the path to this node from the parent map
			std::vector<uvec2> path;
			while (curr != start_pos) {
				path.emplace_back(curr);
				curr = parent.at(curr);
			}
			path.emplace_back(start_pos);
			std::reverse(path.begin(), path.end());
			return path;
		}

		// Otherwise, add all unvisited neighbours to the queue
		// Currently, diagonal movement is not supported
		for (uvec2 movement : { uvec2(1, 0), uvec2(-1, 0), uvec2(0, 1), uvec2(0, -1) }) {
			uvec2 neighbour = curr + movement;
			// Check if neighbour is not already visited, and is walkable
			if (walkable(neighbour) && parent.find(neighbour) == parent.end()) {
				// Enqueue neighbour
				frontier.push(neighbour);
				// Set curr as the parent of neighbour
				parent.emplace(neighbour, curr);
			}
		}

		frontier.pop();
	}

	// Return empty path if no path exists
	return std::vector<uvec2>();
}

TileId MapGeneratorSystem::get_tile_id(uvec2 pos) const
{
	RoomType room_index = current_map().at(pos.y / room_size).at(pos.x / room_size);
	TileId tile_index = room_layouts.at(room_index).at(pos.y % room_size).at(pos.x % room_size);
	return tile_index;
}
