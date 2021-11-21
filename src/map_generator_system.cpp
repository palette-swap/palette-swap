#include "map_generator_system.hpp"
#include "turn_system.hpp"

#include <iostream>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <glm/gtx/hash.hpp>

#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace MapUtility;

MapGeneratorSystem::MapGeneratorSystem(std::shared_ptr<TurnSystem> turns)
	: turns(std::move(turns))
{
	init();
}

void MapGeneratorSystem::init()
{
	load_predefined_level_configurations();
	load_generated_level_configurations();
	load_final_level();
	current_level = -1;
}

void MapGeneratorSystem::load_predefined_level_configurations()
{
	level_configurations.resize(num_predefined_levels);
	// load room layout first, room layouts will be shared among predefine levels
	std::vector<RoomLayout> room_layouts(predefined_room_paths.size());
	for (size_t i = 0; i < predefined_room_paths.size(); i++) {
		RoomLayout& room_mapping = room_layouts.at(i);

		std::ifstream room_file(predefined_room_paths.at(i));
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

	// populate level configurations
	for (size_t i = 0; i < num_predefined_levels; i++) {
		// map layout
		MapLayout& level_mapping = level_configurations.at(i).map_layout;
		std::ifstream level_file(predefined_level_paths.at(i));
		std::string line;
		int col = 0;
		int row = 0;

		while (std::getline(level_file, line)) {
			std::string room_id;
			std::stringstream ss(line);
			while (std::getline(ss, room_id, ',')) {
				level_mapping.at(row).at(col) = (MapUtility::RoomID)std::stoi(room_id);
				col++;
				if (col == room_size) {
					row++;
					col = 0;
				}
			}
		}

		// level snapshot
		std::ifstream config(level_configuration_paths.at(i));
		std::stringstream buffer;
		buffer << config.rdbuf();
		level_configurations.at(i).level_snap_shot = buffer.str();

		// rooms
		level_configurations.at(i).room_layouts = room_layouts;
	}
}

void MapGeneratorSystem::load_final_level()
{
	// room layouts is the same as help level
	// TODO: probably just use a constant among all predefined levels
	LevelConfiguration level_conf;
	level_conf.room_layouts = level_configurations.at(0).room_layouts;

	// map layout
	MapLayout& level_mapping = level_conf.map_layout;
	std::ifstream level_file(final_level_path);
	std::string line;
	int col = 0;
	int row = 0;

	while (std::getline(level_file, line)) {
		std::string room_id;
		std::stringstream ss(line);
		while (std::getline(ss, room_id, ',')) {
			level_mapping.at(row).at(col) = (MapUtility::RoomID)std::stoi(room_id);
			col++;
			if (col == room_size) {
				row++;
				col = 0;
			}
		}
	}

	// level snapshot
	std::ifstream config(final_level_configuration_path);
	std::stringstream buffer;
	buffer << config.rdbuf();
	level_conf.level_snap_shot = buffer.str();

	level_configurations.emplace_back(level_conf);
}

void MapGeneratorSystem::load_generated_level_configurations()
{
	// make sure we have loaded level generation confs
	if (level_generation_confs.empty()) {
		load_level_generation_confs();
	}

	// we are ready to generate the levels
	for (int i = 0; i < level_generation_confs.size(); i++) {
		level_configurations.emplace_back(MapGenerator::generate_level(level_generation_confs.at(i), false));
	}
}

void MapGeneratorSystem::load_level_generation_confs()
{
	// make sure the genertion confs is empty
	if (!level_generation_confs.empty()) {
		level_generation_confs.clear();
	}
	int level_counter = 0;
	while (true) {
		std::ifstream config(level_generation_conf_path(std::to_string(level_counter) + ".json"));
		if (!config.is_open()) {
			return;
		}

		std::stringstream buffer;
		buffer << config.rdbuf();
		rapidjson::Document json_doc;
		json_doc.Parse(buffer.str().c_str());

		MapUtility::LevelGenConf level_gen_conf;
		level_gen_conf.deserialize("/generation_conf", json_doc);
		level_generation_confs.emplace_back(level_gen_conf);

		level_counter++;
	}
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

	// Need to replace with a different component denoting a boss enemy
	if (enemy_component.type == EnemyType::KingMush) {
		enemy_animation.max_frames = 8;
		enemy_animation.speed_adjustment = 0.6;
	} else {
		enemy_animation.max_frames = 4;
	}

	enemy_animation.state = enemy_state_to_animation_state.at(static_cast<size_t>(enemy_component.state));
	registry.emplace<RenderRequest>(entity,
									enemy_type_textures.at(static_cast<int>(enemy_component.type)),
									EFFECT_ASSET_ID::ENEMY,
									GEOMETRY_BUFFER_ID::SMALL_SPRITE,
									true);
	if (enemy_component.team == ColorState::Red) {
		enemy_animation.color = ColorState::Red;
		enemy_animation.display_color = { AnimationUtility::default_enemy_red, 1 };
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

	registry.emplace<Background>(help_picture);
}

const MapLayout& MapGeneratorSystem::get_level_layout(int level) const
{
	assert(level != -1 && (static_cast<unsigned int>(level) < level_configurations.size()));
	return level_configurations.at(level).map_layout;
}

const std::string& MapGeneratorSystem::get_level_snap_shot(int level) const
{
	assert(level != -1 && (static_cast<unsigned int>(level) < level_configurations.size()));
	return level_configurations.at(level).level_snap_shot;
}

const std::vector<MapUtility::RoomLayout>& MapGeneratorSystem::get_level_room_layouts(int level) const
{
	assert(level != -1 && (static_cast<unsigned int>(level) < level_configurations.size()));
	return level_configurations.at(level).room_layouts;
}

const MapLayout& MapGeneratorSystem::current_map() const { return get_level_layout(current_level); }

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

bool MapGeneratorSystem::walkable_and_free(uvec2 pos, bool check_active_color) const
{
	if (!walkable(pos)) {
		return false;
	}
	ColorState active_color = turns->get_active_color();
	if ((active_color == ColorState::Red) != check_active_color) {
		return !std::any_of(
			registry.view<MapPosition>(entt::exclude<Player, RedExclusive, Item, ResourcePickup>).begin(),
			registry.view<MapPosition>(entt::exclude<Player, RedExclusive, Item, ResourcePickup>).end(),
			[pos](const Entity e) { return registry.get<MapPosition>(e).position == pos; });
	}

	return !std::any_of(registry.view<MapPosition>(entt::exclude<Player, BlueExclusive, Item, ResourcePickup>).begin(),
						registry.view<MapPosition>(entt::exclude<Player, BlueExclusive, Item, ResourcePickup>).end(),
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
	RoomID room_index = current_map().at(pos.y / room_size).at(pos.x / room_size);
	return get_tile_id_from_room(current_level, room_index, pos.y % room_size, pos.x % room_size);
}

TileID MapGeneratorSystem::get_tile_id_from_room(int level, RoomID room_id, uint8_t row, uint8_t col) const
{
	return get_level_room_layouts(level)
		.at(static_cast<size_t>(room_id))
		.at(static_cast<size_t>(row * room_size + col));
}

bool MapGeneratorSystem::is_next_level_tile(uvec2 pos) const
{
	return get_tile_id_from_map_pos(pos) == tile_next_level;
}

bool MapGeneratorSystem::is_last_level_tile(uvec2 pos) const
{
	return get_tile_id_from_map_pos(pos) == tile_last_level;
}

bool MapGeneratorSystem::is_trap_tile(uvec2 pos) const
{
	const static std::set<uint8_t> trap_tiles({ 28, 36 });
	return trap_tiles.find(get_tile_id_from_map_pos(pos)) != trap_tiles.end();
}

bool MapGeneratorSystem::is_last_level() const
{
	return (static_cast<size_t>(current_level) == level_configurations.size() - 1);
}
void MapGeneratorSystem::snapshot_level()
{
	rapidjson::Document level_snapshot;
	level_snapshot.Parse(get_level_snap_shot(current_level).c_str());
	rapidjson::CreateValueByPointer(level_snapshot, rapidjson::Pointer("/enemies/0"));

	// Serialize enemies
	int i = 0;
	for (auto [entity, enemy, map_position, stats] : registry.view<Enemy, MapPosition, Stats>().each()) {
		std::string enemy_prefix = "/enemies/" + std::to_string(i++);
		enemy.serialize(enemy_prefix, level_snapshot);
		map_position.serialize(enemy_prefix, level_snapshot);
		stats.serialize(enemy_prefix + "/stats", level_snapshot);
	}

	// save player position
	registry.get<MapPosition>(registry.view<Player>().front()).serialize("/player", level_snapshot);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	level_snapshot.Accept(writer);
	level_configurations.at(current_level).level_snap_shot = buffer.GetString();
}

void MapGeneratorSystem::load_level(int level)
{
	// Load the new map
	create_map(level);
	// Read from snapshots first, if not exists, read from pre-configured file
	const std::string& snapshot = get_level_snap_shot(level);
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

	// update player position
	registry.get<MapPosition>(registry.view<Player>().front()).deserialize("/player", json_doc);

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
	if (is_last_level()) {
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
	if (current_level == 0) {
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
		clear_level();
		init();
	}
	load_level(0);
	current_level = 0;
}

// Creates a room entity, with room type referencing to the predefined room
void MapGeneratorSystem::create_room(vec2 position, MapUtility::RoomID room_id, int level) const
{
	auto entity = registry.create();

	registry.emplace<WorldPosition>(entity, position);
	registry.emplace<Velocity>(entity, 0.f, 0.f);

	Room& room = registry.emplace<Room>(entity);
	room.room_id = room_id;
	room.level = level;

	Animation& tile_animation = registry.emplace<Animation>(entity);
	tile_animation.max_frames = 4;
	tile_animation.state = 0;
	tile_animation.speed_adjustment = 0.5;
}

void MapGeneratorSystem::create_map(int level) const
{
	vec2 middle = { window_width_px / 2, window_height_px / 2 };

	const MapLayout& mapping = get_level_layout(level);
	vec2 top_left_corner_pos = middle - vec2(tile_size * room_size * map_size / 2);
	for (size_t row = 0; row < mapping.size(); row++) {
		for (size_t col = 0; col < mapping[0].size(); col++) {
			vec2 position = top_left_corner_pos + vec2(tile_size * room_size / 2)
				+ vec2(col, row) * tile_size * static_cast<float>(room_size);
			create_room(position, mapping.at(row).at(col), level);
		}
	}
}

const RoomLayout& MapGeneratorSystem::get_room_layout(int level, MapUtility::RoomID room_id) const
{
	return get_level_room_layouts(level).at(static_cast<size_t>(room_id));
}

const std::set<uint8_t>& MapUtility::walkable_tiles()
{
	const static std::set<uint8_t> walkable_tiles({ 0, 14, 20, 28, 36 });
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
void MapGeneratorSystem::start_editing_level()
{
	snapshot_level();
	clear_level();

	// save backups
	level_configurations_backup = level_configurations;
	current_level_backup = current_level;
	// edit from current level or first available generated level if we
	// are currently on a predefined level
	if (current_level < num_predefined_levels) {
		current_level = num_predefined_levels;
	}

	// TODO: Remove once issue#111 is resolved
	static const std::string map_editor_instruction = R"(
In map editing mode.
OPTIONS
	N, load the next level
	B, load previous level
	control + P, save the generation configurations to files
	Q/W increase/decrease generation seed
	A/S increase/decrease total path length(number of rooms on path)
	Z/X increase/decrease number of blocks generated in a room
	E/R increase/decrease number of side rooms
	D/F increase/decrease path complexity in a room
	C/V increase/decrease number of traps in a room
	T/V increase/decrease room smoothness(by running a customized cellular automata)
	G/H increase/decrease enemy density in a room
		)";
	std::cout << map_editor_instruction << std::endl;

	std::cout << "current level: " << current_level << std::endl;
	load_level(current_level);
}
void MapGeneratorSystem::stop_editing_level()
{
	std::cout << "Exiting map editor... " << std::endl;
	clear_level();
	level_configurations = level_configurations_backup;
	current_level = current_level_backup;
	load_level(current_level);
}
void MapGeneratorSystem::edit_next_level()
{
	clear_level();
	current_level++;

	std::cout << "current level: " << current_level << std::endl;
	if (current_level >= level_configurations.size()) {
		assert(level_configurations.size() - num_predefined_levels == level_generation_confs.size());
		level_generation_confs.emplace_back(LevelGenConf());
		level_configurations.emplace_back(
			MapGenerator::generate_level(level_generation_confs.at(current_level - num_predefined_levels), true));
	}
	load_level(current_level);
}
void MapGeneratorSystem::edit_previous_level()
{
	if (current_level == num_predefined_levels) {
		std::cerr << "Cannot edit predefined levels!" << std::endl;
		return;
	}
	clear_level();
	current_level--;

	std::cout << "current level: " << current_level << std::endl;
	load_level(current_level);
}
void MapGeneratorSystem::save_level_generation_confs()
{
	for (size_t i = 0; i < level_generation_confs.size(); i++) {
		rapidjson::Document json_doc;
		level_generation_confs.at(i).serialize("/generation_conf", json_doc);

		// convert to json
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		json_doc.Accept(writer);

		// write to file
		std::ofstream config(level_generation_conf_path(std::to_string(i) + ".json"), std::ofstream::trunc);
		config << buffer.GetString();
		config.close();
	}
	std::cout << "Saved generation configurations!" << std::endl;
}
void MapGeneratorSystem::regenerate_map()
{
	clear_level();
	level_configurations.at(current_level)
		= MapGenerator::generate_level(level_generation_confs.at(current_level - num_predefined_levels), true);
	load_level(current_level);
}
void MapGeneratorSystem::increment_seed()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.seed == UINT_MAX) {
		return;
	}
	curr_conf.seed++;
	std::cout << "current seed: " << curr_conf.seed << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrement_seed()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.seed == 0) {
		return;
	}
	curr_conf.seed--;
	std::cout << "current seed: " << curr_conf.seed << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increment_path_length()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.level_path_length == UINT_MAX) {
		return;
	}
	curr_conf.level_path_length++;
	std::cout << "Current path length: " << curr_conf.level_path_length << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrement_path_length()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.level_path_length == 2) {
		return;
	}
	curr_conf.level_path_length--;
	std::cout << "Current path length: " << curr_conf.level_path_length << std::endl;
	regenerate_map();
}

void MapGeneratorSystem::decrease_room_density()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	// due to precision, comparing with 0.0 won't work
	if (curr_conf.room_density <= 0.05) {
		return;
	}
	curr_conf.room_density -= 0.1;
	std::cout << "Current room density: " << curr_conf.room_density << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increase_room_density()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	// due to precision, comparing with 1.0 won't work
	if (curr_conf.room_density >= 0.95) {
		return;
	}
	curr_conf.room_density += 0.1;
	std::cout << "Current room density: " << curr_conf.room_density << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increase_side_rooms()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.side_room_percentage <= 0.05) {
		return;
	}
	curr_conf.side_room_percentage -= 0.1;
	std::cout << "Current side room percentage: " << curr_conf.side_room_percentage << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrease_side_rooms()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.side_room_percentage >= 0.95) {
		return;
	}
	curr_conf.side_room_percentage += 0.1;
	std::cout << "Current side room percentage: " << curr_conf.side_room_percentage << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increase_room_path_complexity()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.room_path_complexity >= 0.95) {
		return;
	}
	curr_conf.room_path_complexity += 0.1;
	std::cout << "Current room path complexity: " << curr_conf.room_path_complexity << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrease_room_path_complexity()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.room_path_complexity <= 0.15) {
		return;
	}
	curr_conf.room_path_complexity -= 0.1;
	std::cout << "Current room path complexity: " << curr_conf.room_path_complexity << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increase_room_traps_density()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.room_traps_density >= 0.95) {
		return;
	}
	curr_conf.room_traps_density += 0.1;
	std::cout << "Current room traps density: " << curr_conf.room_traps_density << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrease_room_traps_density()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.room_traps_density <= 0.05) {
		return;
	}
	curr_conf.room_traps_density -= 0.1;
	std::cout << "Current room traps density: " << curr_conf.room_traps_density << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increase_room_smoothness()
{
	const static unsigned int max_iteration = 10;
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.room_smoothness == max_iteration) {
		return;
	}
	curr_conf.room_smoothness++;
	std::cout << "Current room smoothness: " << curr_conf.room_smoothness << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrease_room_smoothness()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.room_smoothness == 0) {
		return;
	}
	curr_conf.room_smoothness--;
	std::cout << "Current room smoothness: " << curr_conf.room_smoothness << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::increase_enemy_density()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.enemies_density > 9.95) {
		return;
	}
	curr_conf.enemies_density += 0.1;
	std::cout << "Current enemy density: " << curr_conf.enemies_density << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrease_enemy_density()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.enemies_density <= 0.05) {
		return;
	}
	curr_conf.enemies_density -= 0.1;
	std::cout << "Current enemy density: " << curr_conf.enemies_density << std::endl;
	regenerate_map();
}
