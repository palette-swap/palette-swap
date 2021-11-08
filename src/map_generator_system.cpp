#include "map_generator_system.hpp"

#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include <glm/gtx/hash.hpp>

#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace MapUtility;

MapGeneratorSystem::MapGeneratorSystem()
	: levels(num_levels)
	, level_room_rotations(num_levels)
	, level_snap_shots(num_levels)
	, level_generation_confs(num_levels)
	, room_layouts(num_predefined_rooms)
{
	load_rooms_from_csv();
	//load_levels_from_csv();
	load_level_configurations();
	current_level = -1;
	

	generate_level(0);
}

////////////////////////////////////
// Functions to load different entities, e.g. enemy
static void load_enemy(unsigned int enemy_index, const rapidjson::Document& json_doc)
{
	auto entity = registry.create();
	std::string enemy_prefix = "/enemies/" + std::to_string(enemy_index);
	Enemy& enemy_component = registry.emplace<Enemy>(entity);
	enemy_component.deserialize(enemy_prefix, json_doc);

	MapPosition& map_position_component = registry.emplace<MapPosition>(entity, uvec2(0, 0));
	map_position_component.deserialize(enemy_prefix, json_doc);

	Stats& stats = registry.emplace<Stats>(entity);
	stats.deserialize(enemy_prefix + "/stats", json_doc);

	// Indicates enemy is hittable by objects
	registry.emplace<Hittable>(entity);

	Animation& enemy_animation = registry.emplace<Animation>(entity);
	enemy_animation.max_frames = 4;

	registry.emplace<RenderRequest>(entity,
									enemy_type_textures.at(static_cast<int>(enemy_component.type)),
									  EFFECT_ASSET_ID::ENEMY,
									GEOMETRY_BUFFER_ID::ENEMY,
									true);
	if (enemy_component.team == ColorState::Red) {
		enemy_animation.color = ColorState::Red;
		registry.emplace<Color>(entity, AnimationUtility::default_enemy_red);
	} else if (enemy_component.team == ColorState::Blue) {
		registry.emplace<Color>(entity, AnimationUtility::default_enemy_blue);
		enemy_animation.color = ColorState::Blue;
	} else {
		registry.emplace<Color>(entity, vec3(1, 1, 1));
	}
}

void MapGeneratorSystem::create_picture()
{
	help_picture = registry.create();

	// Create and (empty) player component to be able to refer to other enttities
	registry.emplace<WorldPosition>(help_picture, vec2(window_width_px / 2, window_height_px / 2));

	registry.emplace<RenderRequest>(
		help_picture, TEXTURE_ASSET_ID::HELP_PIC, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE, true);
}

std::vector<int> MapGeneratorSystem::generate_random_path(int start_row, int start_col, int path_length)
{
	std::uniform_int_distribution<int> direction_rand(0, 3);
	std::array<std::array<int, 2>, 4> directions = { {
		{ -1, 0 }, // left
		{ 0, -1 }, // up
		{ 1, 0 }, // right
		{ 0, 1 }, // down
	} };

	// each element in path represents a room position, calculated by row*10+col
	std::vector<int> path = {start_row * 10 + start_col};
	// Keeps a direction_tried set for each room, so we could backtrack
	std::vector<std::set<int>> directions_tried(1);
	int current_row = start_row;
	int current_col = start_col;
	int current_length = 1;

	while (current_length < path_length)
	{
		// we should always be exploring directions of the current last room
		auto & current_directions_tried = directions_tried.back();
		while(current_directions_tried.size() < 4) {
			int direction = direction_rand(random_eng);
			// try a direction we haven't tried before
			if (current_directions_tried.find(direction) != current_directions_tried.end()) {
				continue;
			}
			current_directions_tried.emplace(direction);

			int next_row = current_row + directions.at(direction).at(1);
			int next_col = current_col + directions.at(direction).at(0);
			// make sure we are not out of bounds
			if (next_row >= room_size || next_row < 0 || next_col >= room_size || next_col < 0) {
				continue;
			}

			// make sure the room has not been added to path
			if (std::find(path.begin(), path.end(), (next_row * 10 + next_col )) != path.end()) {
				continue;
			}

			// found a valid direction
			path.emplace_back(next_row * 10 + next_col);
			directions_tried.emplace_back(std::set<int>());
			current_row = next_row;
			current_col = next_col;
			current_length ++;
			break;
		}

		// backtrack if we have checked all four directions but cannot find a valid path
		if (directions_tried.back().size() >= 4) {
			path.pop_back();
			directions_tried.pop_back();
			current_row = path.back() / map_size;
			current_col = path.back() % map_size;
			current_length --;
		}

		if (current_length < 0) {
			// we tried all possibility, but still cannot find a valid path
			fprintf(stderr, "Couldn't generate a path with length %d", path_length);
			assert(0);
		}
	}

	return path;
}

void MapGeneratorSystem::generate_level(int level)
{
	const LevelGenConf& level_gen_conf = level_generation_confs.at(level);
	random_eng.seed(level_gen_conf.generation_seed);

	// Choose a room as the starting position
	std::uniform_int_distribution<int> random_number_distribution(0, 9);
	int row = random_number_distribution(random_eng);
	int col = random_number_distribution(random_eng);
	std::vector<int> paths = generate_random_path(row, col, level_gen_conf.level_path_length);

	// from the path, generate level
	Mapping& level_mapping = levels.at(0);
	for (int row = 0; row < map_size; row++) {
		for (int col = 0; col < map_size; col++) {
			level_mapping.at(row).at(col) = 0;
		}
	}
	auto& room_rotations = level_room_rotations.at(0);
	for (int row = 0; row < map_size; row++) {
		for (int col = 0; col < map_size; col++) {
			room_rotations.at(row).at(col) = Direction::Up;
		}
	}

	// from the generated path, generate rooms
	for (int i = 0 ; i < paths.size(); i ++) {
		// 0: left, 1: up, 2: right, 3: down
		std::set<int> unblocked_direction;
		int cur_row = paths.at(i) / map_size;
		int cur_col = paths.at(i) % map_size;
		if (i + 1 <= paths.size() - 1) {
			int next_row = paths.at(i + 1) / map_size;
			int next_col = paths.at(i + 1) % map_size;
			if (next_row > cur_row) {
				unblocked_direction.emplace(3);
			} else if (next_row < cur_row) {
				unblocked_direction.emplace(1);
			} else if (next_col > cur_col) {
				unblocked_direction.emplace(2);
			} else if (next_col < cur_col) {
				unblocked_direction.emplace(0);
			}
		}
		if (i - 1 >= 0) {
			int last_row = paths.at(i - 1) / map_size;
			int last_col = paths.at(i - 1) % map_size;
			if (last_row > cur_row) {
				unblocked_direction.emplace(3);
			} else if (last_row < cur_row) {
				unblocked_direction.emplace(1);
			} else if (last_col > cur_col) {
				unblocked_direction.emplace(2);
			} else if (last_col < cur_col) {
				unblocked_direction.emplace(0);
			}
		}

		// generate blocks on the blocked direction,
		// generate walkable tiles for other locations
		std::array<uint32_t, map_size * map_size> room_layout;
		for (int room_i = 0; room_i < room_layout.size(); room_i ++) {
			int room_row = room_i / 10;
			int room_col = room_i % 10;
			if (room_row == 0 && (unblocked_direction.find(1) == unblocked_direction.end())) {
				room_layout.at(room_i) = 12;
			} else if (room_row == map_size - 1 && (unblocked_direction.find(3) == unblocked_direction.end())) {
				room_layout.at(room_i) = 12;
			} else if (room_col == 0 && (unblocked_direction.find(0) == unblocked_direction.end())) {
				room_layout.at(room_i) = 12;
			} else if (room_col == map_size - 1 && (unblocked_direction.find(2) == unblocked_direction.end())) {
				room_layout.at(room_i) = 12;
			} else {
				room_layout.at(room_i) = 0;
			}
		}
		room_layouts.emplace_back(std::move(room_layout));
		level_mapping.at(cur_row).at(cur_col) = room_layouts.size() - 1;

	}
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

	return walkable_tiles().find(get_tile_id_from_map_pos(pos)) != walkable_tiles().end();
}

bool MapGeneratorSystem::walkable_and_free(uvec2 pos) const
{
	if (!walkable(pos)) {
		return false;
	}
	return !std::any_of(registry.view<MapPosition>().begin(),
						registry.view<MapPosition>().end(),
						[pos](const Entity e) { return registry.get<MapPosition>(e).position == pos; });
}

bool MapGeneratorSystem::is_wall(uvec2 pos) const
{
	if (!is_on_map(pos)) {
		return false;
	}

	return wall_tiles().find(get_tile_id_from_map_pos(pos)) != wall_tiles().end();
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
			if (neighbour == target || (walkable_and_free(neighbour) && parent.find(neighbour) == parent.end())) {
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

TileID MapGeneratorSystem::get_tile_id_from_map_pos(uvec2 pos) const
{
	RoomType room_index = current_map().at(pos.y / room_size).at(pos.x / room_size);
	Direction room_rotation = level_room_rotations.at(current_level).at(pos.y / room_size).at(pos.x / room_size);
	return get_tile_id_from_room(room_index, pos.y % room_size, pos.x % room_size, room_rotation);
}

void MapGeneratorSystem::load_rooms_from_csv()
{
	for (size_t i = 0; i < room_paths.size(); i++) {
		auto & room_mapping = room_layouts.at(i);

		std::ifstream room_file(room_paths.at(i));
		std::string line;
		int index = 0;

		while (std::getline(room_file, line)) {
			std::string tile_id;
			std::stringstream ss(line);
			while (std::getline(ss, tile_id, ',')) {
				room_mapping.at(index++) = std::stoi(tile_id);
			}
		}
	}
}

TileID MapGeneratorSystem::get_tile_id_from_room(RoomType room_type, uint8_t row, uint8_t col, Direction rotation) const
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
	return static_cast<TileID>(room_layouts.at(room_type).at(static_cast<size_t>(row) * room_size + col));
}

void MapGeneratorSystem::load_levels_from_csv()
{
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
				level_mapping.at(row).at(col) = (MapUtility::RoomType) std::stoi(room_id);
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
		auto& room_rotations = level_room_rotations.at(i);

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

void MapGeneratorSystem::load_level_configurations()
{
	for (size_t i = 0; i < level_configuration_paths.size(); i++) {
		std::ifstream config(level_configuration_paths.at(i));
		std::stringstream buffer;
		buffer << config.rdbuf();
		level_snap_shots.at(i) = buffer.str();
	}
}

Direction MapGeneratorSystem::integer_to_direction(int direction)
{
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

const std::array<std::array<Direction, MapUtility::map_size>, MapUtility::map_size>&
MapGeneratorSystem::current_rooms_rotation() const
{
	return level_room_rotations.at(current_level);
}

bool MapGeneratorSystem::is_next_level_tile(uvec2 pos) const
{
	return get_tile_id_from_map_pos(pos) == tile_next_level;
}

bool MapGeneratorSystem::is_last_level_tile(uvec2 pos) const
{
	return get_tile_id_from_map_pos(pos) == tile_last_level;
}

bool MapGeneratorSystem::is_last_level() const 
{ 
	return (static_cast<size_t>(current_level) == levels.size() - 1);

}
void MapGeneratorSystem::snapshot_level()
{
	rapidjson::Document level_snapshot;
	level_snapshot.Parse(level_snap_shots.at(current_level).c_str());
	rapidjson::CreateValueByPointer(level_snapshot, rapidjson::Pointer("/enemies/0"));

	// Serialize enemies
	int i = 0;
	for (auto [entity, enemy, map_position, stats]: registry.view<Enemy, MapPosition, Stats>().each()) {
		std::string enemy_prefix = "/enemies/" + std::to_string(i++);
		enemy.serialize(enemy_prefix, level_snapshot);
		map_position.serialize(enemy_prefix, level_snapshot);
		stats.serialize(enemy_prefix + "/stats", level_snapshot);
	}

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	level_snapshot.Accept(writer);
	level_snap_shots.at(current_level) = buffer.GetString();
}

void MapGeneratorSystem::load_level(int level)
{
	if (level == num_levels) {
		fprintf(stderr, "Level Cleared!!!!!!");
		assert(0);
	}

	// Load the new map
	create_map(level);
	// Read from snapshots first, if not exists, read from pre-configured file
	const std::string& snapshot = level_snap_shots.at(level);
	assert(!snapshot.empty());

	rapidjson::Document json_doc;
	json_doc.Parse(snapshot.c_str());
	const rapidjson::Value& enemies = json_doc["enemies"];
	assert(enemies.IsArray());

	for (rapidjson::SizeType i = 0; i < enemies.Size(); i++) {
		if (!enemies[i].IsNull()) {
			load_enemy(i, json_doc);
		}
	}

	if (level == 0) {
		if (!registry.valid(help_picture) || !registry.any_of<RenderRequest>(help_picture)) {
			create_picture();
		}
		registry.get<RenderRequest>(help_picture).visible = true;
	}
}

void MapGeneratorSystem::clear_level() const
{
	// Clear the created rooms
	auto room_view = registry.view<Room>();
	registry.destroy(room_view.begin(), room_view.end());

	// Clear the enemies
	auto enemy_view = registry.view<Enemy>();
	registry.destroy(enemy_view.begin(), enemy_view.end());

	if (current_level == 0) {
		if (registry.valid(help_picture) && registry.any_of<RenderRequest>(help_picture)) {
			registry.get<RenderRequest>(help_picture).visible = false;
		}
	}
}

void MapGeneratorSystem::load_next_level()
{
	if (static_cast<size_t>(current_level) == levels.size()) {
		fprintf(stderr, "is already on last level");
		assert(false && "cannot load any new levels");
	}

	if (current_level != -1) {
		// We don't need to snapshot and clear when loading the very first level
		snapshot_level();
		clear_level();
	}
	load_level(++current_level);
}

void MapGeneratorSystem::load_last_level()
{
	if (static_cast<size_t>(current_level) == levels.size()) {
		fprintf(stderr, "is already on first level");
		assert(false && "cannot load any new levels");
	}

	snapshot_level();
	clear_level();
	load_level(--current_level);
}

void MapGeneratorSystem::load_initial_level()
{
	if (current_level != -1) {
		load_level_configurations();
		clear_level();
	}
	load_level(0);
	current_level = 0;
}

// Creates a room entity, with room type referencing to the predefined room
void MapGeneratorSystem::create_room(vec2 position, MapUtility::RoomType roomType, float angle) const
{
	auto entity = registry.create();

	registry.emplace<WorldPosition>(entity, position);
	registry.emplace<Velocity>(entity, 0.f, angle);

	Room& room = registry.emplace<Room>(entity);
	room.type = roomType;
}

void MapGeneratorSystem::create_map(int level) const
{
	vec2 middle = { window_width_px / 2, window_height_px / 2 };

	const MapGeneratorSystem::Mapping& mapping = levels[level];
	const auto& room_rotations = level_room_rotations[level];
	vec2 top_left_corner_pos
		= middle - vec2(tile_size * room_size * map_size / 2);
	for (size_t row = 0; row < mapping.size(); row++) {
		for (size_t col = 0; col < mapping[0].size(); col++) {
			vec2 position = top_left_corner_pos + vec2(tile_size * room_size / 2)
				+ vec2(col, row) * tile_size * static_cast<float>(room_size);
			create_room(position, mapping.at(row).at(col), direction_to_angle(room_rotations.at(row).at(col)));
		}
	}
}

uvec2 MapGeneratorSystem::get_player_start_position() const
{
	rapidjson::Document json_doc;
	json_doc.Parse(level_snap_shots.at(current_level).c_str());

	const auto* x = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/start_position/x"));
	const auto* y = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/start_position/y"));

	return { x->GetInt(), y->GetInt() };
}

uvec2 MapGeneratorSystem::get_player_end_position() const
{
	rapidjson::Document json_doc;
	json_doc.Parse(level_snap_shots.at(current_level).c_str());

	const auto* x = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/end_position/x"));
	const auto* y = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/end_position/y"));

	return { x->GetInt(), y->GetInt() };
}

const std::array<uint32_t, MapUtility::map_size * MapUtility::map_size>&
MapGeneratorSystem::get_room_layout(MapUtility::RoomType type) const
{
	return room_layouts.at(type);
}

const std::set<uint8_t>& MapUtility::walkable_tiles()
{
	const static std::set<uint8_t> walkable_tiles({ 0, 14, 20 });
	return walkable_tiles;
}

const std::set<uint8_t>& MapUtility::wall_tiles()
{
	const static std::set<uint8_t> wall_tiles(
		{ 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 15, 17, 18, 19, 23, 25, 26, 27, 33, 35, 41, 42, 43 });
	return wall_tiles;
}

/////////////////////
// Map editor
void MapGeneratorSystem::regenerate_map()
{
	// TODO: remove hard-coded value
	clear_level();
	room_layouts.resize(num_predefined_rooms);
	generate_level(0);
	load_level(0);
}
void MapGeneratorSystem::increment_seed()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level);
	if (curr_conf.generation_seed == UINT_MAX) {
		return;
	}
	curr_conf.generation_seed++;
	std::cout << "current seed: " << curr_conf.generation_seed << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrement_seed()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level);
	if (curr_conf.generation_seed == 0) {
		return;
	}
	curr_conf.generation_seed--;
	std::cout << "current seed: " << curr_conf.generation_seed << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increment_path_length() {
	LevelGenConf& curr_conf = level_generation_confs.at(current_level);
	if (curr_conf.level_path_length == UINT_MAX) {
		return;
	}
	curr_conf.level_path_length++;
	std::cout << "Current path length: " << curr_conf.level_path_length << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrement_path_length()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level);
	if (curr_conf.level_path_length == 1) {
		return;
	}
	curr_conf.level_path_length--;
	std::cout << "Current path length: " << curr_conf.level_path_length << std::endl;
	regenerate_map();
}

