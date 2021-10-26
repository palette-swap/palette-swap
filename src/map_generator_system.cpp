#include "map_generator_system.hpp"
#include "predefined_room.hpp"

#include "tiny_ecs_registry.hpp"

#include <queue>
#include <unordered_map>
#include <sstream>

#include <glm/gtx/hash.hpp>

using namespace MapUtility;

MapGeneratorSystem::MapGeneratorSystem() { load_rooms_from_csv(); }

// TODO: we want this eventually be procedural generated
void MapGeneratorSystem::generate_levels()
{
	// TODO: generate map procedurally
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

	return walkable_tiles.find(get_tile_id_from_map_pos(pos)) != walkable_tiles.end();
}

bool MapGeneratorSystem::walkable_and_free(uvec2 pos) const
{
	if (!walkable(pos)) {
		return false;
	}
	return !std::any_of(registry.map_positions.components.begin(),
					   registry.map_positions.components.end(),
					   [pos](const MapPosition& pos2) { return pos2.position == pos; });
}

bool MapGeneratorSystem::is_wall(uvec2 pos) const
{
	if (!is_on_map(pos)) {
		return false;
	}

	return wall_tiles.find(get_tile_id_from_map_pos(pos)) != wall_tiles.end();
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
		for (uvec2 neighbour :
			 { curr + uvec2(1, 0), uvec2(curr.x - 1, curr.y), curr + uvec2(0, 1), uvec2(curr.x, curr.y - 1) }) {
			// Check if neighbour is not already visited, and is walkable
			if (walkable_and_free(neighbour) && parent.find(neighbour) == parent.end()) {
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

TileId MapGeneratorSystem::get_tile_id_from_map_pos(uvec2 pos) const
{
	RoomType room_index = current_map().at(pos.y / room_size).at(pos.x / room_size);
	return get_tile_id_from_room(room_index, pos.y % room_size, pos.x % room_size);
}

void MapGeneratorSystem::load_rooms_from_csv()
{
	for (size_t i = 0; i < room_paths.size(); i++) {
		Mapping& room_mapping = room_layouts.at(i);

		std::ifstream room_file(room_paths.at(i));
		std::string line;
		int col = 0;
		int row = 0;

		while (std::getline(room_file, line)) {
			std::string room_id;
			std::stringstream ss(line);
			while (std::getline(ss, room_id, ',')) {
				room_mapping.at(row).at(col) = std::stoi(room_id);
				col++;
				if (col == room_size) {
					row++;
					col = 0;
				}
			}
		}
	}
}

TileId MapGeneratorSystem::get_tile_id_from_room(RoomType room_type, uint8_t row, uint8_t col) const
{
	return room_layouts.at(room_type).at(row).at(col);
}
