#include "map_generator_system.hpp"
#include "predefined_room.hpp"

#include <queue>
#include <unordered_map>

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
	return pos.y < current_map().size() && pos.x < current_map().at(0).size();
}

bool MapGeneratorSystem::walkable(uvec2 pos) const
{
	if (!is_on_map(pos)) {
		return false;
	}
	uint8_t room_index = current_map().at(pos.y / 10).at(pos.x / 10);
	uint8_t tile_index = room_layouts.at(room_index).at(pos.y % 10).at(pos.x % 10);
	return walkable_tiles.find(tile_index) != walkable_tiles.end();
}

// See https://en.wikipedia.org/wiki/Breadth-first_search for algorithm reference
std::vector<uvec2> MapGeneratorSystem::shortest_path(uvec2 start, std::vector<uvec2> end) const
{
	assert(!end.empty());

	std::queue<uvec2> queue;
	// Presence in parent will also be used to track visited
	std::unordered_map<uvec2, uvec2> parent;
	queue.push(start);
	parent.emplace(start, start);
	while (!queue.empty()) {
		uvec2 curr = queue.front();

		// Check if curr is an accepting state
		if (std::find(end.begin(), end.end(), curr) != end.end()) { 
			// Now generate the path to this node from the parent map
			std::vector<uvec2> path;
			while (curr != start) {
				path.emplace_back(curr);
				curr = parent.at(curr);
			}
			path.emplace_back(start);
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
				queue.push(neighbour);
				// Set curr as the parent of neighbour
				parent.emplace(neighbour, curr);
			}
		}

		queue.pop();
	}

	return std::vector<uvec2>();
}
