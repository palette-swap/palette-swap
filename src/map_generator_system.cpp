#include "map_generator_system.hpp"
#include "turn_system.hpp"
#include "ui_system.hpp"

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

MapGeneratorSystem::MapGeneratorSystem(std::shared_ptr<LootSystem> loot_system,
									   std::shared_ptr<TurnSystem> turns,
									   std::shared_ptr<TutorialSystem> tutorials,
									   std::shared_ptr<UISystem> ui_system,
									   std::shared_ptr<SoLoud::Soloud> so_loud)
	: loot_system(std::move(loot_system))
	, turns(std::move(turns))
	, tutorials(std::move(tutorials))
	, ui_system(std::move(ui_system))
	, so_loud(std::move(so_loud))
{
	init();
}

void MapGeneratorSystem::init()
{
	load_predefined_level_configurations();
	load_generated_level_configurations();
	load_final_level();
	current_level = -1;

	spike_wav.load(audio_path("spike.wav").c_str());
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
		level_configurations.at(i).animated_tiles_red.resize(predefined_room_paths.size());
		level_configurations.at(i).animated_tiles_blue.resize(predefined_room_paths.size());
	}

	level_configurations.at(0).animated_tiles_red.at(7).emplace(34,
																AnimatedTile({ true, false, 60, ColorState::All, 1 }));
	level_configurations.at(0).animated_tiles_blue.at(7).emplace(34,
																 AnimatedTile({ true, false, 60, ColorState::All, 1 }));
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

	level_conf.animated_tiles_red.resize(predefined_room_paths.size());
	level_conf.animated_tiles_blue.resize(predefined_room_paths.size());

	level_conf.animated_tiles_red.at(7).emplace(34, AnimatedTile({ true, false, 60, ColorState::All, 1 }));
	level_conf.animated_tiles_blue.at(7).emplace(34, AnimatedTile({ true, false, 60, ColorState::All, 1 }));

	level_configurations.emplace_back(level_conf);
}

void MapGeneratorSystem::load_generated_level_configurations()
{
	// make sure we have loaded level generation confs
	if (level_generation_confs.empty()) {
		load_level_generation_confs();
	}

	// we are ready to generate the levels
	for (auto& conf : level_generation_confs) {
		level_configurations.emplace_back(MapGenerator::generate_level(conf, false));
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
	map_position_component.deserialize(entity, enemy_prefix, json_doc);

	Stats& stats = registry.emplace<Stats>(entity);
	stats.deserialize(enemy_prefix + "/stats", json_doc);

	// Indicates enemy is hittable by objects
	registry.emplace<Hittable>(entity);

	Animation& enemy_animation = registry.emplace<Animation>(entity);
	int enemy_type = static_cast<int>(enemy_component.type);
	AnimationProfile enemy_profile = enemy_type_to_animation_profile.at(enemy_type);
	bool visible = true;

	auto found = std::find(enemy_type_bosses.begin(), enemy_type_bosses.end(), enemy_component.type);
	if (found != enemy_type_bosses.end()) {
		enemy_animation.max_frames = 8;
		enemy_animation.speed_adjustment = 0.6f;
		visible = false;
		registry.emplace<Boss>(entity);
		registry.emplace<Light>(entity, 4.f * MapUtility::tile_size);
		enemy_component.active = false;
	} else {
		enemy_animation.max_frames = 4;
		enemy_animation.travel_offset = enemy_profile.travel_offset;
	}

	enemy_animation.state = enemy_state_to_animation_state.at(static_cast<size_t>(enemy_component.state));
	registry.emplace<RenderRequest>(
		entity, enemy_profile.texture, EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::SMALL_SPRITE, visible);
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
	registry.emplace<WorldPosition>(help_picture, vec2(window_width_px / 2 - 100, window_height_px / 2));

	registry.emplace<RenderRequest>(
		help_picture, TEXTURE_ASSET_ID::HELP_PIC, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE, true);
	registry.emplace<Color>(help_picture, vec3(195.f / 255.f, 161.f / 255.f, 132.f / 255.f));
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

const std::set<MapUtility::RoomID>& MapGeneratorSystem::get_room_at_position(uvec2 pos) const
{
	RoomID room_index = current_map().at(pos.y / room_size).at(pos.x / room_size);
	for (int i = 0; i < level_configurations.at(current_level).big_rooms.size(); i ++) {
		const std::set<RoomID> & connected_rooms = level_configurations.at(current_level).big_rooms.at(i);
		if (connected_rooms.find(room_index) != connected_rooms.end()) {
			return connected_rooms;
		}
	}
	return std::set<RoomID>({room_index});
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

	TileID tile_id = get_tile_id_from_map_pos(pos);

	return (is_floor_tile(tile_id) || is_trap_tile(tile_id) || is_next_level_tile(tile_id)
			|| is_last_level_tile(tile_id) || is_grass_tile(tile_id) || (tile_id == 63 && is_door_tile(tile_id))
			|| (tile_id == 59));
}

bool MapGeneratorSystem::walkable_and_free(Entity entity, uvec2 pos, bool check_active_color) const
{
	ColorState active_color = turns->get_active_color();
	return ((active_color == ColorState::Red) != check_active_color) ? walkable_and_free<RedExclusive>(entity, pos)
																	 : walkable_and_free<BlueExclusive>(entity, pos);
}

template <typename ColorExclusive> bool MapGeneratorSystem::walkable_and_free(Entity entity, uvec2 pos) const
{
	if (!walkable(pos)) {
		return false;
	}
	for (auto [entity_other, map_pos] :
		 registry.view<MapPosition>(entt::exclude<ColorExclusive, Item, ResourcePickup, Environmental>).each()) {
		if (entity != entity_other && map_pos.position == pos) {
			return false;
		}
	}
	for (auto [entity_other, map_size, map_pos] :
		 registry.view<MapHitbox, MapPosition>(entt::exclude<ColorExclusive, Item, ResourcePickup, Environmental>)
			 .each()) {
		auto it = MapArea(map_pos, map_size);
		if (entity == entity_other) {
			continue;
		}
		if (std::any_of(it.begin(), it.end(), [pos](const uvec2& other_pos) { return pos == other_pos; })) {
			return false;
		}
	}
	return true;
}

bool MapGeneratorSystem::is_wall(uvec2 pos) const
{
	if (!is_on_map(pos)) {
		return false;
	}

	return is_wall_tile(get_tile_id_from_map_pos(pos));
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
std::vector<uvec2>
MapGeneratorSystem::shortest_path(Entity entity, uvec2 start_pos, uvec2 target, bool use_a_star) const
{
	if (!use_a_star) {
		return bfs(entity, start_pos, target);
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
			if (!walkable_and_free(entity, neighbour) && neighbour != target) {
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
std::vector<uvec2> MapGeneratorSystem::bfs(Entity entity, uvec2 start_pos, uvec2 target) const
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
			if (neighbour == target
				|| (walkable_and_free(entity, neighbour) && parent.find(neighbour) == parent.end())) {
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

bool MapGeneratorSystem::is_last_level() const
{
	return (static_cast<size_t>(current_level) == level_configurations.size() - 1);
}
void MapGeneratorSystem::snapshot_level()
{
	rapidjson::Document level_snapshot;
	rapidjson::CreateValueByPointer(level_snapshot, rapidjson::Pointer("/enemies/0"));

	// Serialize enemies
	int i = 0;
	for (auto [entity, enemy, map_position, stats] : registry.view<Enemy, MapPosition, Stats>().each()) {
		std::string enemy_prefix = "/enemies/" + std::to_string(i++);
		enemy.serialize(enemy_prefix, level_snapshot);
		map_position.serialize(enemy_prefix, level_snapshot);
		stats.serialize(enemy_prefix + "/stats", level_snapshot);
	}

	// Save big rooms
	rapidjson::Value big_rooms;
	big_rooms.SetArray();
	for (auto [entity, big_room] : registry.view<BigRoom>().each()) {
		rapidjson::Value room_array;
		room_array.SetArray();
		Entity curr = big_room.first_room;
		while (curr != entt::null) {
			BigRoomElement& element = registry.get<BigRoomElement>(curr);
			room_array.PushBack(registry.get<Room>(curr).room_index, level_snapshot.GetAllocator());
			curr = element.next_room;
		}
		big_rooms.PushBack(room_array, level_snapshot.GetAllocator());
	}
	if (level_snapshot.HasMember("big_rooms")) {
		level_snapshot["big_rooms"] = big_rooms;
	} else {
		level_snapshot.AddMember("big_rooms", big_rooms, level_snapshot.GetAllocator());
	}

	// Save visited rooms
	rapidjson::Value visited_rooms;
	visited_rooms.SetArray();
	for (auto [entity, room] : registry.view<Room>().each()) {
		if (room.visible) {
			visited_rooms.PushBack(room.room_index, level_snapshot.GetAllocator());
		}
	}
	if (level_snapshot.HasMember("visited_rooms")) {
		level_snapshot["visited_rooms"] = visited_rooms;
	} else {
		level_snapshot.AddMember("visited_rooms", visited_rooms, level_snapshot.GetAllocator());
	}

	// save player position
	registry.get<MapPosition>(registry.view<Player>().front()).serialize("/player", level_snapshot);

	int item_index = 0;
	for (auto [entity, item, map_position] : registry.view<Item, MapPosition>().each()) {
		std::string item_prefix = "/items/" + std::to_string(i++);
		item.serialize(item_prefix, level_snapshot);
		map_position.serialize(item_prefix, level_snapshot);
	}

	int resource_index = 0;
	for (auto [entity, resource_pickup, map_position] : registry.view<ResourcePickup, MapPosition>().each()) {
		std::string resource_index = "/resources/" + std::to_string(i++);
		resource_pickup.serialize(resource_index, level_snapshot);
		map_position.serialize(resource_index, level_snapshot);
	}

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

	// Enemies
	const rapidjson::Value& enemies = json_doc["enemies"];
	assert(enemies.IsArray());

	for (rapidjson::SizeType i = 0; i < enemies.Size(); i++) {
		if (!enemies[i].IsNull()) {
			load_enemy(i, json_doc);
		}
	}

	// Big rooms
	if (json_doc.HasMember("big_rooms") && json_doc["big_rooms"].IsArray()) {
		const rapidjson::Value& visited_rooms = json_doc["big_rooms"];
		for (rapidjson::SizeType i = 0; i < visited_rooms.Size(); i++) {
			if (visited_rooms[i].IsArray()) {
				Entity big_room = registry.create();
				for (rapidjson::SizeType j = 0; j < visited_rooms[i].Size(); j++) {
					if (visited_rooms[i][j].IsUint()) {
						for (auto [entity, room] : registry.view<Room>().each()) {
							if (room.room_index == visited_rooms[i][j].GetUint()) {
								BigRoom::add_room(big_room, entity);
							}
						}
					}
				}
			}
		}
	}

	// Visited rooms
	if (json_doc.HasMember("visited_rooms") && json_doc["visited_rooms"].IsArray()) {
		const rapidjson::Value& visited_rooms = json_doc["visited_rooms"];
		for (rapidjson::SizeType i = 0; i < visited_rooms.Size(); i++) {
			if (!visited_rooms[i].IsNull()) {
				auto room_index = static_cast<MapUtility::RoomID>(visited_rooms[i].GetInt());
				for (auto [entity, room] : registry.view<Room>().each()) {
					if (room.room_index == room_index) {
						room.visible = true;
						break;
					}
				}
			}
		}
	}

	// update player position
	Entity player = registry.view<Player>().front();
	registry.get<MapPosition>(player).deserialize(player, "/player", json_doc);

	// load items
	if (json_doc.HasMember("items") && json_doc["items"].IsArray()) {
		const rapidjson::Value& items = json_doc["items"];
		for (rapidjson::SizeType i = 0; i < items.Size(); i++) {
			if (!items[i].IsNull()) {
				std::string item_prefix = "/items/" + std::to_string(i);
				Item item_component;
				item_component.deserialize(item_prefix, json_doc);
				MapPosition map_position(uvec2(0, 0));
				map_position.deserialize(entt::null, item_prefix, json_doc);
				loot_system->drop_item(map_position.position, item_component.item_template);
			}
		}
	}

	// load resource
	if (json_doc.HasMember("resources") && json_doc["resources"].IsArray()) {
		const rapidjson::Value& resources = json_doc["resources"];
		for (rapidjson::SizeType i = 0; i < resources.Size(); i++) {
			if (!resources[i].IsNull()) {
				std::string resource_prefix = "/resources/" + std::to_string(i);
				ResourcePickup resource_pickup;
				resource_pickup.deserialize(resource_prefix, json_doc);
				MapPosition map_position(uvec2(0, 0));
				map_position.deserialize(entt::null, resource_prefix, json_doc);
				loot_system->drop_resource_pickup(map_position.position, resource_pickup.resource);
			}
		}
	}

	uvec2 player_initial_position = registry.get<MapPosition>(player).position;
	RoomID starting_room
		= get_level_layout(level).at(player_initial_position.y / room_size).at(player_initial_position.x / room_size);
	animated_room_buffer.emplace(starting_room);

	if (level == 0) {
		if (!registry.valid(help_picture) || !registry.any_of<RenderRequest>(help_picture)) {
			create_picture();
		}
		registry.get<RenderRequest>(help_picture).visible = true;
	}
}

void MapGeneratorSystem::clear_level()
{
	// Clear the created rooms
	auto room_view = registry.view<Room>();
	registry.destroy(room_view.begin(), room_view.end());
	auto big_room_view = registry.view<BigRoom>();
	registry.destroy(big_room_view.begin(), big_room_view.end());

	// Clear the enemies
	auto enemy_view = registry.view<Enemy>();
	registry.destroy(enemy_view.begin(), enemy_view.end());

	// Clear the drops
	auto item_view = registry.view<Item, MapPosition>();
	registry.destroy(item_view.begin(), item_view.end());
	auto pickup_view = registry.view<ResourcePickup, MapPosition>();
	registry.destroy(pickup_view.begin(), pickup_view.end());

	if (current_level == 0) {
		if (registry.valid(help_picture) && registry.any_of<RenderRequest>(help_picture)) {
			registry.get<RenderRequest>(help_picture).visible = false;
		}
	}
	animated_room_buffer.clear();
}

bool MapGeneratorSystem::load_next_level()
{
	if (is_last_level()) {
		fprintf(stderr, "is already on last level");
		return false;
	}

	if (current_level != -1) {
		// We don't need to snapshot and clear when loading the very first level
		snapshot_level();
		clear_level();
	}
	load_level(++current_level);
	return true;
}

bool MapGeneratorSystem::load_last_level()
{
	if (current_level == 0) {
		fprintf(stderr, "is already on first level");
		return false;
	}

	snapshot_level();
	clear_level();
	load_level(--current_level);
	return true;
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
void MapGeneratorSystem::create_room(vec2 position, MapUtility::RoomID room_id, int level, uint index) const
{
	auto entity = registry.create();

	registry.emplace<WorldPosition>(entity, position);
	registry.emplace<Velocity>(entity, 0.f, 0.f);

	Room& room = registry.emplace<Room>(entity);
	room.room_id = room_id;
	room.level = level;
	room.room_index = index;

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
			create_room(position, mapping.at(row).at(col), level, row * MapUtility::map_size + col);
		}
	}
}

const RoomLayout& MapGeneratorSystem::get_room_layout(int level, MapUtility::RoomID room_id) const
{
	return get_level_room_layouts(level).at(static_cast<size_t>(room_id));
}

std::vector<std::map<int, AnimatedTile>>& MapGeneratorSystem::get_level_animated_tiles(int level)
{
	ColorState& inactive_color = registry.get<PlayerInactivePerception>(registry.view<Player>().front()).inactive;

	LevelConfiguration& level_conf = level_configurations.at(level);
	return (inactive_color == ColorState::Blue) ? level_conf.animated_tiles_red : level_conf.animated_tiles_blue;
}

void MapGeneratorSystem::set_all_inactive_colours(ColorState inactive_color)
{
	Entity player = registry.view<Player>().front();
	PlayerInactivePerception& player_perception = registry.get<PlayerInactivePerception>(player);
	player_perception.inactive = inactive_color;
}

MapGeneratorSystem::MoveState MapGeneratorSystem::move_player_to_tile(uvec2 from_pos, uvec2 to_pos)
{
	if (is_next_level_tile(get_tile_id_from_map_pos(to_pos))) {
		if (is_last_level()) {
			return MapGeneratorSystem::MoveState::EndOfGame;
		}

		load_next_level();
		set_all_inactive_colours(turns->get_inactive_color());
		return MapGeneratorSystem::MoveState::NextLevel;
	}
	if (is_last_level_tile(get_tile_id_from_map_pos(to_pos))) {
		load_last_level();
		set_all_inactive_colours(turns->get_inactive_color());
		return MapGeneratorSystem::MoveState::LastLevel;
	}

	RoomID from_room = current_map().at(from_pos.y / room_size).at(from_pos.x / room_size);
	RoomID to_room = current_map().at(to_pos.y / room_size).at(to_pos.x / room_size);
	if (from_room != to_room) {
		animated_room_buffer.emplace(to_room);
	}

	Entity player_entity = registry.view<Player>().front();
	ColorState& inactive_color = registry.get<PlayerInactivePerception>(player_entity).inactive;
	LevelConfiguration& level_conf = level_configurations.at(current_level);
	auto& animated_tiles
		= (inactive_color == ColorState::Blue) ? level_conf.animated_tiles_red : level_conf.animated_tiles_blue;

	int tile_position_in_room = (to_pos.y % room_size) * room_size + to_pos.x % room_size;
	const auto& current_animated_tile = animated_tiles.at(to_room).find(tile_position_in_room);
	if (current_animated_tile != animated_tiles.at(to_room).end()) {
		if (current_animated_tile->second.is_trigger) {
			current_animated_tile->second.activated = true;
		}

		if (is_spike_tile(current_animated_tile->second.tile_id)) {
			so_loud->play(spike_wav);
			Stats& stats = registry.get<Stats>(player_entity);
			stats.health -= max(0, 5 + stats.damage_modifiers.at((size_t)DamageType::Physical));
		}
		if (is_fire_tile(current_animated_tile->second.tile_id)) {
			Stats& stats = registry.get<Stats>(player_entity);
			stats.health -= max(0, 10 + stats.damage_modifiers.at((size_t)DamageType::Fire));
		}
	}

	registry.get<MapPosition>(player_entity).position = to_pos;
	return MapGeneratorSystem::MoveState::Success;
	;
}

bool MapGeneratorSystem::interact_with_surrounding_tile(Entity player)
{
	uvec2 player_position = registry.get<MapPosition>(player).position;

	const static std::array<std::array<int, 2>, 4> directions = { { { -1, 0 }, { 0, -1 }, { 1, 0 }, { 0, 1 } } };
	int player_row = player_position.y / map_size;
	int player_col = player_position.x % map_size;

	ColorState& inactive_color = registry.get<PlayerInactivePerception>(registry.view<Player>().front()).inactive;

	auto& level_animated_tiles_red = level_configurations.at(current_level).animated_tiles_red;
	auto& level_animated_tiles_blue = level_configurations.at(current_level).animated_tiles_blue;
	Inventory& inventory = registry.get<Inventory>(player);

	for (const auto& direction : directions) {
		int target_row = player_position.y + direction[0];
		int target_col = player_position.x + direction[1];

		if (target_row < 0 || target_row >= map_size * room_size || target_col < 0
			|| target_col >= map_size * room_size) {
			continue;
		}

		RoomID target_room = current_map().at(target_row / room_size).at(target_col / room_size);
		auto& room_animated_tiles = (inactive_color == ColorState::Blue) ? level_animated_tiles_red.at(target_room)
																		 : level_animated_tiles_blue.at(target_room);
		auto& room_animated_tiles_inactive = (inactive_color == ColorState::Blue)
			? level_animated_tiles_blue.at(target_room)
			: level_animated_tiles_red.at(target_room);

		const auto& animated_tile
			= room_animated_tiles.find((target_row % room_size) * room_size + target_col % room_size);
		if (animated_tile != room_animated_tiles.end() && animated_tile->second.usage_count > 0) {
			if (animated_room_buffer.find(target_room) == animated_room_buffer.end()) {
				animated_room_buffer.emplace(target_room);
			}
			if (is_door_tile(animated_tile->second.tile_id)) {
				if (inventory.resources.at((size_t)Resource::Key) == 0) {
					continue;
				}
				inventory.resources.at((size_t)Resource::Key)--;
				ui_system->update_resource_count();
				tutorials->destroy_tooltip(TutorialTooltip::LockedSeen);
			}
			if (is_locked_chest_tile(animated_tile->second.tile_id)) {
				if (inventory.resources.at((size_t)Resource::Key) == 0) {
					continue;
				}
				inventory.resources.at((size_t)Resource::Key)--;
				ui_system->update_resource_count();
				loot_system->drop_loot(player_position, 4.0, 2);
				tutorials->destroy_tooltip(TutorialTooltip::LockedSeen);
			}
			if (is_chest_tile(animated_tile->second.tile_id)) {
				loot_system->drop_loot(player_position, 2.0, 1);
				tutorials->destroy_tooltip(TutorialTooltip::ChestSeen);
			}

			animated_tile->second.activated = true;
			animated_tile->second.usage_count--;

			// update the state in the other dimension as well
			if (animated_tile->second.dimension == ColorState::All) {
				const auto& animated_tile_inactive
					= room_animated_tiles_inactive.find((target_row % room_size) * room_size + target_col % room_size);
				animated_tile_inactive->second.usage_count--;
				if (animated_tile_inactive->second.usage_count == 0) {
					animated_tile_inactive->second.frame = animated_tile_inactive->second.max_frames - 1;
				}
			}
		}
	}
	return true;
}

void MapGeneratorSystem::step(float elapsed_ms)
{
	Entity player_entity = registry.view<Player>().front();
	LevelConfiguration& level_conf = level_configurations.at(current_level);
	auto& animated_tiles = get_level_animated_tiles(current_level);

	std::vector<RoomID> animation_completed_rooms;

	for (RoomID room_index : animated_room_buffer) {
		bool all_animations_completed = true;
		std::map<int, AnimatedTile>& room_animated_tile = animated_tiles.at(room_index);
		for (auto& animated_tile_iter : room_animated_tile) {
			if (!animated_tile_iter.second.activated) {
				continue;
			}

			animated_tile_iter.second.elapsed_time += elapsed_ms;
			if (animated_tile_iter.second.elapsed_time > 100.f / animated_tile_iter.second.speed_adjustment) {
				animated_tile_iter.second.elapsed_time = 0;
				if (animated_tile_iter.second.usage_count == 0
					&& animated_tile_iter.second.frame + 1 == animated_tile_iter.second.max_frames) {
					animated_tile_iter.second.activated = false;
					continue;
				}
				animated_tile_iter.second.frame
					= ((animated_tile_iter.second.frame) + 1) % animated_tile_iter.second.max_frames;

				level_conf.room_layouts.at(room_index).at(animated_tile_iter.first)
					= animated_tile_iter.second.tile_id + animated_tile_iter.second.frame;
				if (animated_tile_iter.second.is_trigger && animated_tile_iter.second.frame == 0) {
					animated_tile_iter.second.activated = false;
				}
			}
			if ((animated_tile_iter.second.is_trigger && animated_tile_iter.second.activated)
				|| animated_tile_iter.second.frame != 0) {
				all_animations_completed = false;
			}
		}
		if (all_animations_completed) {
			animation_completed_rooms.emplace_back(room_index);
		}
	}

	uvec2& player_position = registry.get<MapPosition>(player_entity).position;
	RoomID current_room_index = current_map().at(player_position.y / room_size).at(player_position.x / room_size);
	for (RoomID room_index : animation_completed_rooms) {
		if (room_index != current_room_index) {
			animated_room_buffer.erase(room_index);
		}
	}
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
	U/I increase/decrease room difficulty
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
	if (current_level >= level_configurations.size() - 1) {
		assert(level_configurations.size() - num_predefined_levels - 1 == level_generation_confs.size());
		level_generation_confs.emplace_back(LevelGenConf());
		level_configurations.insert(
			level_configurations.end() - 1,
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
	if (curr_conf.side_room_percentage >= 9.95) {
		return;
	}
	curr_conf.side_room_percentage += 1.0;
	std::cout << "Current side room percentage: " << curr_conf.side_room_percentage << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrease_side_rooms()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.side_room_percentage <= 0.05) {
		return;
	}
	curr_conf.side_room_percentage -= 1.0;
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
void MapGeneratorSystem::increase_level_difficulty()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.level_difficulty == INT_MAX) {
		return;
	}
	curr_conf.level_difficulty++;
	std::cout << "Current level difficulty: " << curr_conf.level_difficulty << std::endl;
	regenerate_map();
}
void MapGeneratorSystem::decrease_level_difficulty()
{
	LevelGenConf& curr_conf = level_generation_confs.at(current_level - num_predefined_levels);
	if (curr_conf.level_difficulty == 1) {
		return;
	}
	curr_conf.level_difficulty--;
	std::cout << "Current level difficulty: " << curr_conf.level_difficulty << std::endl;
	regenerate_map();
}
