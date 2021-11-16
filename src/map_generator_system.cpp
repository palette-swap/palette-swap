#include "map_generator_system.hpp"

#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <glm/gtx/hash.hpp>

#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace MapUtility;

MapGeneratorSystem::MapGeneratorSystem()
	: room_layouts()
	, levels(num_levels)
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
static void load_enemy(unsigned int enemy_index, const rapidjson::Document& json_doc)
{
	auto entity = registry.create();
	std::string enemy_prefix = "/enemies/" + std::to_string(enemy_index);
	Enemy& enemy_component = registry.emplace<Enemy>(entity);
	enemy_component.deserialize(enemy_prefix, json_doc);
	// Loads enemy behaviour based on pre-designated enemy type
	enemy_component.behaviour = enemy_type_to_behaviour.at(static_cast<int>(enemy_component.type));

	MapPosition& map_position_component = registry.emplace<MapPosition>(entity, uvec2(0, 0));
	map_position_component.deserialize(enemy_prefix, json_doc);

	Stats& stats = registry.emplace<Stats>(entity);
	stats.deserialize(enemy_prefix + "/stats", json_doc);

	// Indicates enemy is hittable by objects
	registry.emplace<Hittable>(entity);

	Animation& enemy_animation = registry.emplace<Animation>(entity);
	enemy_animation.max_frames = 4;
	enemy_animation.state = enemy_state_to_animation_state.at(static_cast<size_t>(enemy_component.state));
	registry.emplace<RenderRequest>(entity,
									enemy_type_textures.at(static_cast<int>(enemy_component.type)),
									EFFECT_ASSET_ID::ENEMY,
									GEOMETRY_BUFFER_ID::SMALL_SPRITE,
									true);
	if (enemy_component.team == ColorState::Red) {
		enemy_animation.color = ColorState::Red;
		enemy_animation.display_color = {AnimationUtility::default_enemy_red,1};
		registry.emplace<Color>(entity, AnimationUtility::default_enemy_red);
		registry.emplace<RedExclusive>(entity);
	} else if (enemy_component.team == ColorState::Blue) {
		registry.emplace<Color>(entity, AnimationUtility::default_enemy_blue);
		enemy_animation.color = ColorState::Blue;
		enemy_animation.display_color = { AnimationUtility::default_enemy_blue, 1 };
		registry.emplace<BlueExclusive>(entity);
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
