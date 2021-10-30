#include "map_generator_system.hpp"

#include "tiny_ecs_registry.hpp"

#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <glm/gtx/hash.hpp>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/pointer.h"

using namespace MapUtility;

MapGeneratorSystem::MapGeneratorSystem()
	: levels(num_levels)
	, level_room_rotations(num_levels)
	, level_snap_shots(num_levels)
{
	load_rooms_from_csv(); 
	load_levels_from_csv();
	load_level_configurations();
	current_level = -1;
}

////////////////////////////////////
// Functions to load different entities, e.g. enemy
static void load_enemy(int enemy_index, const rapidjson::Document & json_doc)
{
	auto entity = Entity();
	std::string enemy_prefix = "/enemies/" + std::to_string(enemy_index);
	Enemy& enemy_component = registry.enemies.emplace(entity);
	enemy_component.deserialize(enemy_prefix, json_doc);

	MapPosition& map_position_component = registry.map_positions.emplace(entity, uvec2(0, 0));
	map_position_component.deserialize(enemy_prefix, json_doc);

	Stats& stats = registry.stats.emplace(entity);
	stats.deserialize(enemy_prefix + "/stats", json_doc);

	// Indicates enemy is hittable by objects
	registry.hittables.emplace(entity);

	// TODO: Switch out basic enemy type based on input (Currently Defaulted to Slug)
	registry.render_requests.insert(entity,
									{ TEXTURE_ASSET_ID::SLIME, EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::ENEMY });
	registry.colors.insert(entity, { 1, 1, 1 });

	// TODO: Combine with render_requests above, so animation system handles render requests as a middleman
	// Update animation number using animation system max frames, instead of this static number
	Animation& enemy_animation = registry.animations.emplace(entity);
	enemy_animation.max_frames = 4;
}

void MapGeneratorSystem::create_picture()
{
	help_picture = Entity();

	// Create and (empty) player component to be able to refer to other enttities
	registry.world_positions.emplace(help_picture, vec2(window_width_px / 2, window_height_px / 2));

	registry.render_requests.insert(
		help_picture, { TEXTURE_ASSET_ID::HELP_PIC, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });
	// registry.colors.insert(entity, { 1, 1, 1 });
}


// TODO: we want this eventually be procedural generated
void MapGeneratorSystem::generate_levels()
{
	// TODO: generate map procedurally
	
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
		for (uvec2 neighbour : { curr + uvec2(1, 0),
								 uvec2(curr.x - 1, curr.y),
								 curr + uvec2(0, 1),
								 uvec2(curr.x, curr.y - 1) }) {
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

void MapGeneratorSystem::load_level_configurations() {
	for (size_t i = 0; i < level_configuration_paths.size(); i++) {
		std::ifstream config(level_configuration_paths.at(i));
		std::stringstream buffer;
		buffer << config.rdbuf();
		level_snap_shots.at(i) = buffer.str();
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

bool MapGeneratorSystem::is_next_level_tile(uvec2 pos) const
{
	return get_tile_id_from_map_pos(pos) == tile_next_level;
}

bool MapGeneratorSystem::is_last_level_tile(uvec2 pos) const
{
	return get_tile_id_from_map_pos(pos) == tile_last_level;
}

void MapGeneratorSystem::snapshot_level()
{
	rapidjson::Document level_snapshot;
	level_snapshot.Parse(level_snap_shots.at(current_level).c_str());
	rapidjson::CreateValueByPointer(level_snapshot, rapidjson::Pointer("/enemies/0"));

	// Serialize enemies
	for (size_t i = 0; i < registry.enemies.size(); i++) {
		Entity entity = registry.enemies.entities[i];
		std::string enemy_prefix = "/enemies/" + std::to_string(i);

		Enemy& enemy = registry.enemies.get(entity);
		enemy.serialize(enemy_prefix, level_snapshot);

		MapPosition& map_position = registry.map_positions.get(entity);
		map_position.serialize(enemy_prefix, level_snapshot);

		Stats& stats = registry.stats.get(entity);
		stats.serialize(enemy_prefix + "/stats", level_snapshot);
	}

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	level_snapshot.Accept(writer);
	level_snap_shots.at(current_level) = buffer.GetString();
}

void MapGeneratorSystem::load_level(int level) {
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

	for (auto i = 0; i < enemies.Size(); i++) {
		if (!enemies[i].IsNull()) {
			load_enemy(i, json_doc);
		}
	}

	if (level == 0) {
		if (help_picture == Entity::undefined() || !registry.render_requests.has(help_picture)) {
			create_picture();
		}
		registry.render_requests.get(help_picture).visible = true;
	}
}

void MapGeneratorSystem::clear_level() {

	// Clear the created rooms
	std::list<Entity> entities_to_clear;
	for (int i = 0; i < registry.rooms.size(); i++) {
		Entity room_entity = registry.rooms.entities[i];
		entities_to_clear.emplace_back(room_entity);
	}
	// Clear the enemies
	for (int i = 0; i < registry.enemies.size(); i++) {
		Entity enemy_entity = registry.enemies.entities[i];
		entities_to_clear.emplace_back(enemy_entity);
	}

	for (Entity e : entities_to_clear) {
		registry.remove_all_components_of(e);
	}

	if (current_level == 0) {
		registry.render_requests.get(help_picture).visible = false;
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
Entity MapGeneratorSystem::create_room(vec2 position, MapUtility::RoomType roomType, float angle) const
{
	auto entity = Entity();

	registry.world_positions.emplace(entity, position);
	registry.velocities.emplace(entity, 0, angle);

	Room& room = registry.rooms.emplace(entity);
	room.type = roomType;

	// Place the rooms at bottom
	RenderRequest & request = registry.render_requests.push_front(entity);
	request.used_texture = TEXTURE_ASSET_ID::TILE_SET;
	request.used_effect = EFFECT_ASSET_ID::TILE_MAP;
	request.used_geometry = GEOMETRY_BUFFER_ID::ROOM;

	return entity;
}

void MapGeneratorSystem::create_map(int level) const {
	vec2 middle = { window_width_px / 2, window_height_px / 2 };

	const MapGeneratorSystem::Mapping& mapping = levels[level];
	const auto& room_rotations = level_room_rotations[level];
	vec2 top_left_corner_pos = middle
		- vec2(tile_size * room_size * map_size / 2,
			   tile_size * room_size * map_size / 2);
	for (size_t row = 0; row < mapping.size(); row++) {
		for (size_t col = 0; col < mapping[0].size(); col++) {
			vec2 position = top_left_corner_pos + vec2(tile_size * room_size / 2, tile_size * room_size / 2)
				+ vec2(col * tile_size * room_size, row * tile_size * room_size);
			create_room(position, mapping.at(row).at(col), direction_to_angle(room_rotations.at(row).at(col)));
		}
	}
}

uvec2 MapGeneratorSystem::get_player_start_position() const
{
	uvec2 pos;
	rapidjson::Document json_doc;
	json_doc.Parse(level_snap_shots.at(current_level).c_str());

	const auto* x = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/start_position/x"));
	pos.x = x->GetInt();
	const auto* y = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/start_position/y"));
	pos.y = y->GetInt();
	return pos;
}

uvec2 MapGeneratorSystem::get_player_end_position() const
{
	uvec2 pos;
	rapidjson::Document json_doc;
	json_doc.Parse(level_snap_shots.at(current_level).c_str());

	const auto* x = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/end_position/x"));
	pos.x = x->GetInt();
	const auto* y = rapidjson::GetValueByPointer(json_doc, rapidjson::Pointer("/player/end_position/y"));
	pos.y = y->GetInt();
	return pos;
}
