#include "map_generator_system.hpp"
#include "predefined_room.hpp"

#include "tiny_ecs_registry.hpp"

#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <glm/gtx/hash.hpp>

using namespace MapUtility;

MapGeneratorSystem::MapGeneratorSystem() { load_rooms_from_csv(); }

// TODO: we want this eventually be procedural generated
void MapGeneratorSystem::generate_levels()
{
	// TODO: generate map procedurally
	load_levels_from_csv();
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

std::vector<uvec2> make_path(std::unordered_map<uvec2, uvec2>& parent, uvec2 start_pos, uvec2 target)
{
	// Now generate the path to this node from the parent map
	std::vector<uvec2> path;
	uvec2 curr = target;
	while (curr != start_pos) {
		path.emplace_back(curr);
		curr = parent.at(curr);
	}
	path.emplace_back(start_pos);
	std::reverse(path.begin(), path.end());
	return path;
}

// See https://en.wikipedia.org/wiki/A*_search_algorithm for algorithm reference
std::vector<uvec2> MapGeneratorSystem::shortest_path(uvec2 start_pos, uvec2 target, bool use_a_star) const
{
	if (!use_a_star) {
		return bfs(start_pos, target);
	}
	std::unordered_map<uvec2, uvec2> parent;
	std::unordered_map<uvec2, float> min_score;
	std::unordered_set<uvec2> visited;

	const auto& score = [target, &min_score](uvec2 a) {
		vec2 d = vec2(a) - vec2(target);
		return min_score[a] + abs(d.x) + abs(d.y);
	};
	const auto& min_expected
		= [](const std::pair<uvec2, float>& a, const std::pair<uvec2, float>& b) { return a.second > b.second; };
	std::priority_queue<std::pair<uvec2, float>, std::vector<std::pair<uvec2, float>>, decltype(min_expected)> open_set(
		min_expected);
	min_score[start_pos] = 0;
	open_set.emplace(start_pos, score(start_pos));

	while (!open_set.empty()) {
		const std::pair<uvec2, float> curr = open_set.top();
		if (curr.first == target) {
			return make_path(parent, start_pos, target);
		}
		if (visited.find(curr.first) != visited.end()) {
			open_set.pop();
			continue;
		}
		for (uvec2 neighbour : { curr.first + uvec2(1, 0),
								 uvec2(curr.first.x - 1, curr.first.y),
								 curr.first + uvec2(0, 1),
								 uvec2(curr.first.x, curr.first.y - 1) }) {
			if (!walkable_and_free(neighbour) && neighbour != target) {
				continue;
			}
			float tentative_score = curr.second + 1.f; // NOTE: Can support variable costs here
			auto prev_score = min_score.find(neighbour);
			if (prev_score == min_score.end() || prev_score->second > tentative_score) {
				parent.emplace(neighbour, curr.first);
				min_score[neighbour] = tentative_score;
				open_set.emplace(neighbour, tentative_score);
			}
		}
		visited.emplace(curr.first);
		open_set.pop();
	}

	// Return empty path if no path exists
	return std::vector<uvec2>();
}

// See https://en.wikipedia.org/wiki/Breadth-first_search for algorithm reference
std::vector<uvec2> MapGeneratorSystem::bfs(uvec2 start_pos, uvec2 target) const
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
			return make_path(parent, start_pos, target);
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
	Direction room_rotation = level_room_rotations.at(current_level).at(pos.y / room_size).at(pos.x / room_size);
	return get_tile_id_from_room(room_index, pos.y % room_size, pos.x % room_size, room_rotation);
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
			std::string tile_id;
			std::stringstream ss(line);
			while (std::getline(ss, tile_id, ',')) {
				room_mapping.at(row).at(col) = std::stoi(tile_id);
				col++;
				if (col == room_size) {
					row++;
					col = 0;
				}
			}
		}
	}
}

TileId MapGeneratorSystem::get_tile_id_from_room(RoomType room_type, uint8_t row, uint8_t col, Direction rotation) const
{
	switch (rotation) {
	case Direction::Up:
		break;
	case Direction::Left: {
		uint8_t tmp = row;
		row = col;
		col = room_size - 1 - tmp;
	} break;
	case Direction::Down: {
		row = room_size - 1 - row;
		col = room_size - 1 - col;
	} break;
	case Direction::Right: {
		uint8_t tmp = row;
		row = room_size - 1 - col;
		col = tmp;
	} break;
	default:
		break;
	}
	return room_layouts.at(room_type).at(row).at(col);
}

void MapGeneratorSystem::load_levels_from_csv() {
	// Pre-allocate vector size
	levels.resize(level_paths.size());
	for (size_t i = 0; i < level_paths.size(); i++) {
		Mapping& level_mapping = levels.at(i);

		std::ifstream level_file(level_paths.at(i));
		std::string line;
		int col = 0;
		int row = 0;

		while (std::getline(level_file, line)) {
			std::string room_id;
			std::stringstream ss(line);
			while (std::getline(ss, room_id, ',')) {
				level_mapping.at(row).at(col) = std::stoi(room_id);
				col++;
				if (col == room_size) {
					row++;
					col = 0;
				}
			}
		}
	}

	// load rooms rotations
	level_room_rotations.resize(room_rotation_paths.size());
	for (size_t i = 0; i < room_rotation_paths.size(); i++) {
		auto & room_rotations = level_room_rotations.at(i);

		std::ifstream rotations_file(room_rotation_paths.at(i));
		std::string line;
		int col = 0;
		int row = 0;

		while (std::getline(rotations_file, line)) {
			std::string room_id;
			std::stringstream ss(line);
			while (std::getline(ss, room_id, ',')) {
				room_rotations.at(row).at(col) = integer_to_direction(std::stoi(room_id));
				col++;
				if (col == room_size) {
					row++;
					col = 0;
				}
			}
		}
	}
}

Direction MapGeneratorSystem::integer_to_direction(int direction) {
	switch (direction) {
	case 0:
		return Direction::Up;
	case 1:
		return Direction::Right;
	case 2:
		return Direction::Down;
	case 3:
		return Direction::Left;
	default:
		assert(false && "Unexpected direction");
	}
	return Direction::Up;
}

const std::array<std::array<Direction, MapUtility::map_size>, MapUtility::map_size> &
MapGeneratorSystem::current_rooms_rotation() const
{
	return level_room_rotations.at(current_level);
}