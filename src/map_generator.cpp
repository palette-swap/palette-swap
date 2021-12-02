#include "map_generator.hpp"
#include "components.hpp"

#include <algorithm>
#include <set>
#include <sstream>

#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace MapUtility;

// Enemy templates
static std::array<rapidjson::Document, (size_t)EnemyType::EnemyCount> enemy_templates;
static bool enemy_templates_loaded = false;

// room templates
static const size_t num_room_templates = 3;
static std::array<rapidjson::Document, num_room_templates> template_room_snapshot;
static std::array<RoomLayout, num_room_templates> template_room_layout;
static bool room_templates_loaded = false;

static std::string enemy_template_path(const std::string& name)
{
	return data_path() + "/enemies/" + std::string(name);
};
static std::string room_template_path(const std::string & name)
{
	return data_path() + "/room_templates/" + std::string(name);
}
const static std::array<std::string, num_room_templates> room_templates_paths = {
	room_template_path("entrance.json"), room_template_path("exit.json"), room_template_path("event.json"),
};

const static std::array<std::string, (size_t)EnemyType::EnemyCount> enemy_template_paths = {
	enemy_template_path("TrainingDummy.json"), enemy_template_path("Slime.json"),	 enemy_template_path("Raven.json"),
	enemy_template_path("Armor.json"),		   enemy_template_path("TreeAnt.json"),	 enemy_template_path("Wraith.json"),
	enemy_template_path("Drake.json"),		   enemy_template_path("Mushroom.json"), enemy_template_path("Spider.json"),
	enemy_template_path("Clone.json"),		   enemy_template_path("KingMush.json"),
};

static void load_enemies_from_file()
{
	for (size_t i = 0; i < enemy_template_paths.size(); i++) {
		std::ifstream config(enemy_template_paths.at(i));
		std::stringstream buffer;
		buffer << config.rdbuf();
		std::string enemy_i = buffer.str();

		enemy_templates.at(i).Parse(enemy_i.c_str());
	}
}

static void load_room_templates()
{
	for (size_t i = 0; i < room_templates_paths.size(); i++) {
		std::ifstream config(room_templates_paths.at(i));
		std::stringstream buffer;
		buffer << config.rdbuf();
		std::string room_template = buffer.str();

		template_room_snapshot.at(i).Parse(room_template.c_str());
		rapidjson::Document & json_doc = template_room_snapshot.at(i);

		for (size_t tile_index = 0; tile_index < room_size * room_size; tile_index ++) {
			template_room_layout.at(i).at(tile_index) = json_doc["room_layout"][tile_index].GetUint();
		}
	}
	room_templates_loaded = true;
}

RoomLayout MapGenerator::get_template_room_layout(MapGenerator::RoomType room_type)
{
	assert(static_cast<uint8_t>(room_type) >= static_cast<uint8_t>(RoomType::Entrance));
	if (!room_templates_loaded) {
		load_room_templates();
	}

	return template_room_layout.at(static_cast<uint8_t>(room_type) - static_cast<uint8_t>(RoomType::Entrance));
}

static void
add_enemy_to_level_snapshot(rapidjson::Document& level_snap_shot, ColorState team, EnemyType type, uvec2 map_pos)
{
	if (!enemy_templates_loaded) {
		load_enemies_from_file();
		enemy_templates_loaded = true;
	}

	if (!level_snap_shot.HasMember("enemies")) {
		rapidjson::Value enemy_array(rapidjson::kArrayType);
		level_snap_shot.AddMember("enemies", enemy_array, level_snap_shot.GetAllocator());
	}

	Enemy enemy_type;
	enemy_type.deserialize("", enemy_templates.at(static_cast<size_t>(type)), false);
	enemy_type.team = team;
	enemy_type.type = type;
	enemy_type.nest_map_pos = map_pos;

	Stats enemy_stats;
	enemy_stats.deserialize("/stats", enemy_templates.at(static_cast<size_t>(type)));

	MapPosition map_position(map_pos);

	rapidjson::SizeType enemy_index = level_snap_shot["enemies"].Size();
	std::string enemy_prefix = "/enemies/" + std::to_string(enemy_index);
	enemy_type.serialize(enemy_prefix, level_snap_shot);
	enemy_stats.serialize(enemy_prefix + "/stats", level_snap_shot);
	map_position.serialize(enemy_prefix, level_snap_shot);
}

////////////////////////////////////
// Helpers for procedural generation
const static std::map<Direction, std::array<int, 2>> direction_vec = {
	{ Direction::Left, { -1, 0 } }, // left
	{ Direction::Up, { 0, -1 } }, // up
	{ Direction::Right, { 1, 0 } }, // right
	{ Direction::Down, { 0, 1 } }, // down
};

static Direction opposite_direction(Direction d)
{
	switch (d) {
	case Direction::Left:
		return Direction::Right;
	case Direction::Up:
		return Direction::Down;
	case Direction::Right:
		return Direction::Left;
	case Direction::Down:
		return Direction::Up;
	default:
		assert(0 && "Cannot obtain opposite direction of undefined direction");
		return Direction::Undefined;
	}
}

// Tile ID contants
const static uint8_t solid_block_tile = 12;
const static uint8_t floor_tile = 0;
const static uint8_t void_tile = 10;
const static uint8_t next_level_tile = 14;
const static uint8_t last_level_tile = 15;

// TODO:remove chest tile from trap tiles
const static std::array<uint8_t, 3> trap_tiles = { 28, 36, 44 };
const static std::map<TileID, AnimatedTile> animated_tiles = {
	{28, AnimatedTile({true, false, 28})}, // spike
	{36, AnimatedTile({false, true, 36})}, // fire
	{52, AnimatedTile({true, false, 52, 1.0f})}, // grass
	{44, AnimatedTile({true, false, 44, 0.6f, 1})}, // chest
	{20, AnimatedTile({false, true, 20})}, // torch
};

const static std::array<uint8_t, 8> floor_tiles = { 0, 8, 16, 24, 32, 40, 52 }; 
const static std::array<uint8_t, 1> obstacle_tiles = { 56 };

// masks to define the property of a certain tile, use values out of uint8 to avoid duplications 
const static uint32_t room_floor_mask = 256;
const static uint32_t room_wall_mask = 257;
const static uint32_t room_obstacle_mask = 258;
const static uint32_t room_loot_mask = 259;

// room entrance size on each open side
const static int room_entrance_size = 2;
// the start and end position of the entrance
const static int room_entrance_start = (room_size - room_entrance_size) / 2;
const static int room_entrance_end = (room_size + room_entrance_size) / 2 - 1;

// there are a total of 12 types of boundary tiles, 4 are stright block tiles, each has a staight line
// on each direction, the other 8 are corner tiles, which will be decided by how 2 of the four straight tiles are placed
const static uint8_t boundary_tile_top = 2;
const static uint8_t boundary_tile_left = 9;
const static uint8_t boundary_tile_bot = 18;
const static uint8_t boundary_tile_right = 11;
// inner corner tiles
const static uint8_t boundary_tile_inner_tl = 1; // top left
const static uint8_t boundary_tile_inner_tr = 3; // top right
const static uint8_t boundary_tile_inner_bl = 17; // bot left
const static uint8_t boundary_tile_inner_br = 19; // bot right
// outer corner tiles
const static uint8_t boundary_tile_outer_tl = 25; // top left
const static uint8_t boundary_tile_outer_tr = 26; // top right
const static uint8_t boundary_tile_outer_bl = 33; // bot left
const static uint8_t boundary_tile_outer_br = 34; // bot right

// randomly generate a floor tile from given floor tiles
const static uint8_t generate_random_floor_tile(std::default_random_engine & random_eng)
{
	std::uniform_int_distribution<int> floor_tile_dist(0, floor_tiles.size() - 1);
	return static_cast<uint8_t>(floor_tiles.at(floor_tile_dist(random_eng)));
}

const static uint8_t generate_random_obstacle_tile(std::default_random_engine & random_eng)
{
	std::uniform_int_distribution<int> obstacle_tile_dist(0, obstacle_tiles.size() - 1);
	return static_cast<uint8_t>(obstacle_tiles.at(obstacle_tile_dist(random_eng)));
}

const static uint8_t generate_boundary_tile(const RoomLayout & room_layout, size_t tile_index)
{
	size_t tile_row = tile_index / room_size;
	size_t tile_col = tile_index % room_size;
	if (tile_row < 1 || room_layout.at(tile_index - room_size) == void_tile) {
		if (tile_col < 1 || room_layout.at(tile_index - 1) == void_tile)
			return boundary_tile_inner_tl;
		if (tile_col + 1 > room_size - 1 || room_layout.at(tile_index + 1) == void_tile)
			return boundary_tile_inner_tr;
		return boundary_tile_top;
	}
	if (tile_row + 1 > room_size - 1 || room_layout.at(tile_index + room_size) == void_tile) {
		if (tile_col < 1 || room_layout.at(tile_index - 1) == void_tile)
			return boundary_tile_inner_bl;
		if (tile_col + 1 > room_size - 1 || room_layout.at(tile_index + 1) == void_tile)
			return boundary_tile_inner_br;
		return boundary_tile_bot;
	}
	if (tile_col < 1 || room_layout.at(tile_index - 1) == void_tile) {
		return boundary_tile_left;
	}
	if (tile_col + 1 > room_size - 1 || room_layout.at(tile_index + 1) == void_tile) {
		return boundary_tile_right;
	}
	if (room_layout.at(tile_index + 1) == room_wall_mask) {
		if (room_layout.at(tile_index + room_size) == room_wall_mask)
			return boundary_tile_outer_tl;
		if (room_layout.at(tile_index - room_size) == room_wall_mask)
			return boundary_tile_outer_bl;
	}
	if (room_layout.at(tile_index - 1) == room_wall_mask) {
		if (room_layout.at(tile_index + room_size) == room_wall_mask)
			return boundary_tile_outer_tr;
		if (room_layout.at(tile_index - room_size) == room_wall_mask)
			return boundary_tile_outer_br;
	}

	return solid_block_tile;
}

// update boundary tiles to render more naturally, purely for visual effects
const static void update_room_tiles(RoomLayout &room_layout, const std::set<Direction>& open_directions, std::default_random_engine & random_eng)
{
	RoomLayout original_room_layout = room_layout;
	for (size_t i = 0; i < original_room_layout.size(); i++) {
		uint32_t tile_mask = original_room_layout.at(i);
		switch (tile_mask)
		{
		case room_floor_mask: {
			room_layout.at(i) = generate_random_floor_tile(random_eng);
			break;
		}
		case room_wall_mask: {
			room_layout.at(i) = generate_boundary_tile(original_room_layout, i);
			break;
		}
		case room_obstacle_mask: {
			room_layout.at(i) = generate_random_obstacle_tile(random_eng);
			break;
		}
		default:
			break;
		}
	}

	// update the entrance tiles
	if (open_directions.find(Direction::Up) != open_directions.end()) {
		if (original_room_layout.at(room_entrance_start - 1 + room_size) == room_wall_mask) {
			room_layout.at(room_entrance_start - 1) = boundary_tile_left;
		} else {
			room_layout.at(room_entrance_start - 1) = boundary_tile_outer_br;
		}
		if (original_room_layout.at(room_entrance_end + 1 + room_size) == room_wall_mask) {
			room_layout.at(room_entrance_end + 1) = boundary_tile_right;
		} else {
			room_layout.at(room_entrance_end + 1) = boundary_tile_outer_bl;
		}
	}
	if (open_directions.find(Direction::Down) != open_directions.end()) {
		if (original_room_layout.at(room_size * (room_size - 2) + room_entrance_start - 1) == room_wall_mask) {
			room_layout.at(room_size * (room_size - 1) + room_entrance_start - 1) = boundary_tile_left;
		} else {
			room_layout.at(room_size * (room_size - 1) + room_entrance_start - 1) = boundary_tile_outer_tr;
		}
		if (original_room_layout.at(room_size * (room_size - 2) + room_entrance_end + 1) == room_wall_mask) {
			room_layout.at(room_size * (room_size - 1) + room_entrance_end + 1) = boundary_tile_right;
		} else {
			room_layout.at(room_size * (room_size - 1) + room_entrance_end + 1) = boundary_tile_outer_tl;
		}
	}
	if (open_directions.find(Direction::Left) != open_directions.end()) {
		if (original_room_layout.at(room_size * (room_entrance_start - 1) + 1) == room_wall_mask) {
			room_layout.at(room_size * (room_entrance_start - 1)) = boundary_tile_top;
		} else {
			room_layout.at(room_size * (room_entrance_start - 1)) = boundary_tile_outer_br;
		}
		if (original_room_layout.at(room_size * (room_entrance_end + 1) + 1) == room_wall_mask) {
			room_layout.at(room_size * (room_entrance_end + 1)) = boundary_tile_bot;
		} else {
			room_layout.at(room_size * (room_entrance_end + 1)) = boundary_tile_outer_tr;
		}
	}
	if (open_directions.find(Direction::Right) != open_directions.end()) {
		if (original_room_layout.at(room_size * (room_entrance_start) - 1 - 1) == room_wall_mask) {
			room_layout.at(room_size * (room_entrance_start) - 1) = boundary_tile_top;
		} else {
			room_layout.at(room_size * (room_entrance_start) - 1) = boundary_tile_outer_bl;
		}
		if (original_room_layout.at(room_size * (room_entrance_end + 2) - 1 - 1) == room_wall_mask) {
			room_layout.at(room_size * (room_entrance_end + 2) - 1) = boundary_tile_bot;
		} else {
			room_layout.at(room_size * (room_entrance_end + 2) - 1) = boundary_tile_outer_tl;
		}
	}

	room_layout = room_layout;
}

// customized cellular automata algorithm to smooth the room out
// TODO: incorporate rock and grass generation to cellular automata
static void smooth_room(RoomLayout& curr_layout, uint iterations, const std::set<int>& critical_locations)
{
	RoomLayout updated_layout = curr_layout;
	auto get_neighbour_walls = [](int tile_row, int tile_col, const RoomLayout& room_layout) {
		int wall_count = 0;
		for (int neighbour_row = tile_row - 1; neighbour_row <= tile_row + 1; neighbour_row++) {
			for (int neighbour_col = tile_col - 1; neighbour_col <= tile_col + 1; neighbour_col++) {
				if (neighbour_row == tile_row && neighbour_col == tile_col) {
					continue;
				}
				if (neighbour_row < 0 || neighbour_row >= room_size || neighbour_col < 0 || neighbour_col >= room_size
					|| (room_layout.at(neighbour_row * room_size + neighbour_col)
						== static_cast<uint32_t>(room_wall_mask))
					|| room_layout.at(neighbour_row * room_size + neighbour_col) == static_cast<uint32_t>(void_tile)) {
					wall_count++;
				}
			}
		}
		return wall_count;
	};

	for (uint i = 0; i < iterations; i++) {
		// each iteration is broken into two steps, smoothing out and shrinking
		// 1. smooth room out based on neighbouring tiles
		for (int tile_position = 0; tile_position < curr_layout.size(); tile_position++) {
			if (critical_locations.find(tile_position) != critical_locations.end()
				|| curr_layout.at(tile_position) == next_level_tile
				|| curr_layout.at(tile_position) == last_level_tile) {
				continue;
			}
			int tile_row = tile_position / 10;
			int tile_col = tile_position % 10;
			int num_walls_around = get_neighbour_walls(tile_row, tile_col, curr_layout);

			if (num_walls_around > 3) {
				updated_layout.at(tile_position) = room_wall_mask;
			} else {
				updated_layout.at(tile_position) = room_floor_mask;
			}
		}

		// 2. shrink room from outside
		for (int tile_position = 0; tile_position < curr_layout.size(); tile_position++)  {
			int tile_row = tile_position / 10;
			int tile_col = tile_position % 10;
			int num_walls_around = get_neighbour_walls(tile_row, tile_col, updated_layout);

			if (num_walls_around == 8) {
				updated_layout.at(tile_position) = void_tile;
			}
		}
		curr_layout = updated_layout;
	}
}

void MapGenerator::generate_room(MapGenerator::PathNode * starting_node,
								 MapUtility::LevelGenConf level_gen_conf,
								 MapUtility::LevelConfiguration & level_conf,
								 RoomGenerationEngines random_engs,
								 bool is_debugging)
{
	std::set<Direction> open_directions = starting_node->get_room_open_directions();
	RoomType room_type = starting_node->room_type;
	// max generation values
	const static double max_side_path_probability = 0.9;
	const static double max_traps_density = 0.3;

	// Room templates
	const static std::map<RoomType, RoomLayout> room_templates = {
		{ RoomType::Entrance,
		  {
			  0, 0, 0, 0, 0,  0, 0,	 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,	0,	0,	0, 0, 0,
			  0, 0, 0, 0, 0,  0, 0,	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,	0,	0,	0, 0, 0,
			  0, 0, 0, 0, 0,  0, 0,	 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,	0,	0,	0, 0, 0,
		  } },
		{ RoomType::Exit,
		  {
			  0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0,
			  0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0,
			  0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0,
		  } },
	};

	// get a random direction when generating path within a room.
	// When generating, direction that is opposite to the starting direction will be preferred,
	// the likelihood of choosing a direction that's vertical to the starting direction can be
	// increase by increasing room path complexity
	auto get_next_direction = [&](Direction starting_direction) {
		std::bernoulli_distribution side_path_dist(max_side_path_probability * level_gen_conf.room_path_complexity);
		if (!side_path_dist(random_engs.general_eng)) {
			return opposite_direction(starting_direction);
		}

		// randomly choose a side to side step
		std::bernoulli_distribution side_dist(0.5);
		bool left_side = side_dist(random_engs.general_eng);

		if (starting_direction == Direction::Left || starting_direction == Direction::Right) {
			return left_side ? Direction::Up : Direction::Down;
		}
		if (starting_direction == Direction::Up || starting_direction == Direction::Down) {
			return left_side ? Direction::Left : Direction::Right;
		}
		assert(0);
		return Direction::Undefined;
	};

	// add a straight path within a room from starting location to end location
	auto add_straight_path = [](std::set<int>& current_path, int from_position, int to_position) {
		int from_row = from_position / 10;
		int from_col = from_position % 10;
		int to_row = to_position / 10;
		int to_col = to_position % 10;
		assert((from_row == to_row) || (from_col == to_col));
		while (from_row != to_row) {
			from_row += (to_row > from_row) ? 1 : -1;
			current_path.emplace(from_row * 10 + from_col);
		}
		while (from_col != to_col) {
			from_col += (to_col > from_col) ? 1 : -1;
			current_path.emplace(from_row * 10 + from_col);
		}
	};

	// based on room open directions, decide if the room is a corridar
	auto is_corridor_room = [&]() {
		return (open_directions.size() == 2 && room_type == RoomType::Critical
				&& open_directions.find(opposite_direction(*(open_directions.begin()))) != open_directions.end());
	};

	std::uniform_int_distribution<int> corridar_width_dist(4, 6);
	int corridar_width = corridar_width_dist(random_engs.general_eng);

	int first_row = 0;
	int last_row = room_size - 1;
	int first_col = 0;
	int last_col = room_size - 1;
	if (is_corridor_room()) {
		if (open_directions.find(Direction::Up) != open_directions.end()) {
			first_col = (room_size - corridar_width) / 2;
			last_col = (room_size + corridar_width) / 2 - 1;
		}
		if (open_directions.find(Direction::Left) != open_directions.end()) {
			first_row = (room_size - corridar_width) / 2;
			last_row = (room_size + corridar_width) / 2 - 1;
		}
	}

	// checks if a tile is on the room entrance path, the entrance path is the middle tiles at the open sides of a room
	auto is_on_entrance_path = [&](int room_tile_position) {
		int room_tile_row = room_tile_position / 10;
		int room_tile_col = room_tile_position % 10;
		return ((open_directions.find(Direction::Up) != open_directions.end()
				 && (room_tile_row == first_row
					 && (room_entrance_start <= room_tile_col && room_tile_col <= room_entrance_end)))
				|| (open_directions.find(Direction::Left) != open_directions.end()
					&& (room_tile_col == first_col
						&& (room_entrance_start <= room_tile_row && room_tile_row <= room_entrance_end)))
				|| (open_directions.find(Direction::Down) != open_directions.end()
					&& (room_tile_row == last_row
						&& (room_entrance_start <= room_tile_col && room_tile_col <= room_entrance_end)))
				|| (open_directions.find(Direction::Right) != open_directions.end()
					&& (room_tile_col == last_col
						&& (room_entrance_start <= room_tile_row && room_tile_row <= room_entrance_end))));
	};

	// check if a tile is on the boundary, this is used for generating the boundary tiles(walls)
	auto is_boundary_tile = [&](int room_tile_position) {
		return (!is_on_entrance_path(room_tile_position)
				&& ((room_tile_position / room_size == first_row) || (room_tile_position / room_size == last_row)
					|| (room_tile_position % room_size == first_col) || (room_tile_position % room_size == last_col)));
	};

	// Check if a tile is outside of a room
	auto is_outside_tile = [&](int room_tile_position) {
		return (first_row > (room_tile_position / room_size) || (room_tile_position / room_size) > last_row
				|| first_col > (room_tile_position % room_size) || (room_tile_position % room_size) > last_col);
	};

	// get an arbitary starting poistion given the starting direction
	auto get_starting_position = [&](Direction starting_direction) {
		std::uniform_int_distribution<int> starting_pos_dist(room_entrance_start, room_entrance_end);
		int starting_pos = starting_pos_dist(random_engs.general_eng);
		if (starting_direction == Direction::Left) {
			return room_size * starting_pos + first_col;
		}
		if (starting_direction == Direction::Right) {
			return room_size * starting_pos + last_col;
		}
		if (starting_direction == Direction::Up) {
			return room_size * first_row + starting_pos;
		}
		if (starting_direction == Direction::Down) {
			return room_size * last_row + starting_pos;
		}
		assert(0);
		return -1;
	};

	// start generating path that connects all open sides
	std::set<Direction> sides_to_connect = open_directions;

	// a set is sufficient to save the path as we only need to make sure we don't generate a cycle, and is more
	// efficient
	std::set<int> critical_locations;
	Direction starting_direction = *(sides_to_connect.begin());
	// we use previous room position to obtain the next room position
	int previous_room_position = get_starting_position(starting_direction);

	RoomLayout room_layout {};

	if (room_type == RoomType::Critical) {
		room_layout.fill(room_floor_mask);
		while (!sides_to_connect.empty()) {
			Direction next_direction = get_next_direction(starting_direction);
			int next_room_position = (critical_locations.empty())
				? previous_room_position
				: previous_room_position + room_size * direction_vec.at(static_cast<Direction>(next_direction)).at(1)
					+ direction_vec.at(static_cast<Direction>(next_direction)).at(0);

			// make sure we are within the room boundary and we are not re-generating an existing tile
			if (next_room_position < 0 || next_room_position >= room_size * room_size
				|| is_boundary_tile(next_room_position)
				|| critical_locations.find(next_room_position) != critical_locations.end()) {
				continue;
			}

			int next_row = next_room_position / room_size;
			int next_col = next_room_position % room_size;

			// check if current position is aligned with any opening sides, we directly go straight towards it
			if (next_row >= room_entrance_start && next_row <= room_entrance_end) {
				if (sides_to_connect.find(Direction::Left) != sides_to_connect.end()) {
					add_straight_path(critical_locations, next_room_position, next_row * room_size + first_col);
					sides_to_connect.erase(Direction::Left);
				}
				if (sides_to_connect.find(Direction::Right) != sides_to_connect.end()) {
					add_straight_path(critical_locations, next_room_position, next_row * room_size + last_col);
					sides_to_connect.erase(Direction::Right);
				}
			}
			if (next_col >= room_entrance_start && next_col <= room_entrance_end) {
				if (sides_to_connect.find(Direction::Up) != sides_to_connect.end()) {
					add_straight_path(critical_locations, next_room_position, first_row * room_size + next_col);
					sides_to_connect.erase(Direction::Up);
				}
				if (sides_to_connect.find(Direction::Down) != sides_to_connect.end()) {
					add_straight_path(critical_locations, next_room_position, last_row * room_size + next_col);
					sides_to_connect.erase(Direction::Down);
				}
			}

			// add the current position and continue searching
			critical_locations.emplace(next_room_position);
			previous_room_position = next_room_position;
		}
	} else if (room_type == RoomType::Side) {
		/* Might be okay to not do anything for side rooms, or we could have some templates */
	} else {
		room_layout = get_template_room_layout(room_type);
	}

	std::bernoulli_distribution blocks_dist(0.05);
	std::bernoulli_distribution spawn_traps_dist(0.05);
	std::uniform_int_distribution<int> traps_dist(0, trap_tiles.size() - 1);

	for (int room_row = 0; room_row < room_size; room_row++) {
		for (int room_col = 0; room_col < room_size; room_col++) {
			int room_index = room_row * room_size + room_col;
			// also add entrance paths to critical locations
			if (is_on_entrance_path(room_index)) {
				critical_locations.emplace(room_index);
			}

			if (is_outside_tile(room_index)) {
				room_layout.at(room_index) = void_tile;
			} else if (is_boundary_tile(room_index)) {
				room_layout.at(room_index) = room_wall_mask;
			} else if (room_type == RoomType::Critical || room_type == RoomType::Side) {
				if (critical_locations.find(room_index) == critical_locations.end()
						&& blocks_dist(random_engs.general_eng)) {
					room_layout.at(room_index) = room_obstacle_mask;
				} else if (spawn_traps_dist(random_engs.traps_eng)) {
					int trap_tile_index = traps_dist(random_engs.general_eng);
					room_layout.at(room_index) = trap_tiles.at(trap_tile_index);
				} else {
					room_layout.at(room_index) = room_floor_mask;
				}
			}
		}
	}

	// smooth the room out based on specified iterations
	if (room_type != RoomType::Event) {
		smooth_room(room_layout, level_gen_conf.room_smoothness, critical_locations);
	}

	// update boundary tiles
	random_engs.general_eng.seed(level_gen_conf.seed);
	update_room_tiles(room_layout, open_directions, random_engs.general_eng);

	// for (int i : critical_locations) {
	// 	room_layout.at(i) = 12;
	// }
	level_conf.room_layouts.emplace_back(room_layout);

	// populate animated tiles
	std::map<int, AnimatedTile> animated_tiles_red;
	std::map<int, AnimatedTile> animated_tiles_blue;
	for (size_t room_tile_index = 0; room_tile_index < room_layout.size(); room_tile_index ++) {
		const auto & animated_tile_iter = animated_tiles.find(room_layout.at(room_tile_index));
		if (animated_tile_iter != animated_tiles.end()) {
			animated_tiles_red.emplace(room_tile_index, animated_tile_iter->second);
		}
	}
	
	level_conf.animated_tiles_red.emplace_back(animated_tiles_red);
	level_conf.animated_tiles_blue.emplace_back(animated_tiles_blue);
}

void MapGenerator::generate_enemies(MapUtility::LevelGenConf level_gen_conf,
									const MapUtility::RoomLayout& room_layout,
									int room_position_on_map,
									rapidjson::Document& level_snap_shot,
									std::default_random_engine& enemies_random_eng_red,
									std::default_random_engine& general_random_eng_blue)
{

	const static double max_enemies_density = 0.1;
	const static uint8_t floor_tile = 0;
	// generate enemies
	std::bernoulli_distribution enemies_dist(max_enemies_density * level_gen_conf.enemies_density);

	int room_map_row = room_position_on_map / 10;
	int room_map_col = room_position_on_map % 10;

	// We don't spawn clone enemies
	std::uniform_int_distribution<int> enemy_types_dist(1, static_cast<int>(EnemyType::Clone));

	for (int room_row = 0; room_row < room_size; room_row++) {
		for (int room_col = 0; room_col < room_size; room_col++) {
			int room_index = room_row * room_size + room_col;
			if (enemies_dist(enemies_random_eng_red) && room_layout.at(room_index) % 8 == 0) {
				add_enemy_to_level_snapshot(
					level_snap_shot,
					ColorState::Red,
					static_cast<EnemyType>(enemy_types_dist(enemies_random_eng_red)),
					uvec2(room_map_col * room_size + room_col, room_map_row * room_size + room_row));
			}
			if (enemies_dist(general_random_eng_blue) && room_layout.at(room_index) % 8 == 0) {
				add_enemy_to_level_snapshot(
					level_snap_shot,
					ColorState::Blue,
					static_cast<EnemyType>(enemy_types_dist(general_random_eng_blue)),
					uvec2(room_map_col * room_size + room_col, room_map_row * room_size + room_row));
			}
		}
	}
}

// get the open direction of the room given the start room to end room, note the two rooms are expect to be neighbours
static Direction get_open_direction(int from_room_index, int to_room_index)
{
	int from_row = from_room_index / 10;
	int from_col = from_room_index % 10;
	int to_row = to_room_index / 10;
	int to_col = to_room_index % 10;

	if (to_row < from_row) {
		return Direction::Up;
	}
	if (to_row > from_row) {
		return Direction::Down;
	}
	if (to_col < from_col) {
		return Direction::Left;
	}
	if (to_col > from_col) {
		return Direction::Right;
	}
	return Direction::Undefined;
};

// get all open directions of a room
std::set<Direction> MapGenerator::PathNode::get_room_open_directions() const
{
	std::set<Direction> open_directions;
	int room_row = position / 10;
	int room_col = position % 10;

	if (parent != nullptr) {
		open_directions.emplace(get_open_direction(position, parent->position));
	}
	for (const auto & child : children) {
		open_directions.emplace(get_open_direction(position, child->position));
	}
	return open_directions;
};

bool MapGenerator::generate_path_from_node(PathNode * curr_room,
										   int path_length,
										   std::set<int> & visited_rooms,
										   std::default_random_engine & random_eng,
										   MapGenerator::RoomType room_type)
{
	if (path_length == 0) {
		if (room_type == RoomType::Critical) {
			curr_room->room_type = RoomType::Exit;
		} else if (curr_room->room_type == RoomType::Side) {
			std::bernoulli_distribution generate_event_room_dist(0.5);
			if (true || generate_event_room_dist(random_eng)) {
				curr_room->room_type = RoomType::Event;
			}
		}
		return true;
	}

	// randomly choose one of the four directions
	std::uniform_int_distribution<int> direction_rand(0, 3);

	int current_row = curr_room->position / 10;
	int current_col = curr_room->position % 10;

	std::array<Direction, 4> directions({ Direction::Left, Direction::Up, Direction::Right, Direction::Down });
	// randonly shuffle order of four directions
	std::shuffle(std::begin(directions), std::end(directions), random_eng);

	bool found_valid_path = false;

	for (auto direction : directions) {
		int next_row = current_row + direction_vec.at(direction).at(1);
		int next_col = current_col + direction_vec.at(direction).at(0);
		if ((next_row < map_size && next_row >= 0 && next_col < map_size && next_col >= 0)
			&& (visited_rooms.find(next_row * map_size + next_col) == visited_rooms.end()))
		{
			int next_room_position = next_row * map_size + next_col;
			MapGenerator::PathNode * next_room = new PathNode(next_room_position, true, room_type);
			curr_room->children.emplace(next_room);
			next_room->parent = curr_room;

			visited_rooms.emplace(next_room_position);
			if (generate_path_from_node(next_room, path_length - 1, visited_rooms, random_eng, room_type)) {
				found_valid_path = true;
				break;
			}

			// backtrack
			visited_rooms.erase(next_room_position);
			curr_room->children.erase(next_room);
			delete(next_room);
		}
	}

	return found_valid_path;
}

void MapGenerator::traverse_path_and_generate_rooms(MapGenerator::PathNode * starting_node,
													LevelGenConf & level_gen_conf,
													LevelConfiguration & level_conf,
													rapidjson::Document & level_snap_shot,
													MapGenerator::RoomGenerationEngines & room_rand_eng)
{
	// TODO: add event/chest/hidden room geneartion here
	generate_room(starting_node, level_gen_conf, level_conf, room_rand_eng, false);
	level_conf.map_layout.at(starting_node->position / 10).at(starting_node->position % 10) = static_cast<RoomID>(level_conf.room_layouts.size() - 1);

	generate_enemies(level_gen_conf, level_conf.room_layouts.back(), starting_node->position, level_snap_shot, room_rand_eng.enemy_random_eng_red, room_rand_eng.enemy_random_eng_blue);

	// refresh general random engine
	room_rand_eng.general_eng.seed(level_gen_conf.seed);
	for (auto & child : starting_node->children) {
		traverse_path_and_generate_rooms(child, level_gen_conf, level_conf, level_snap_shot, room_rand_eng);
	}
}

LevelConfiguration MapGenerator::generate_level(LevelGenConf level_gen_conf, bool is_debugging)
{
	LevelConfiguration level_conf;
	// prepare the random engines
	std::default_random_engine random_eng;
	random_eng.seed(level_gen_conf.seed);

	// initialize all rooms to be void first
	RoomLayout void_room;
	void_room.fill(void_tile);
	level_conf.room_layouts.emplace_back(void_room);
	level_conf.animated_tiles_red.resize(1);
	level_conf.animated_tiles_blue.resize(1);
	for (int row = 0; row < map_size; row++) {
		for (int col = 0; col < map_size; col++) {
			level_conf.map_layout.at(row).at(col) = 0;
		}
	}

	// 1. Start procedural generation by choosing a random position to place the first room
	std::uniform_int_distribution<int> random_number_distribution(0, map_size - 1);

	int current_row = random_number_distribution(random_eng);
	int current_col = random_number_distribution(random_eng);
	PathNode * starting_room = new PathNode(current_row * map_size + current_col, true, RoomType::Entrance);

	std::set<int> visited_rooms {current_row * map_size + current_col};
	if (!generate_path_from_node(starting_room, level_gen_conf.level_path_length - 1, visited_rooms, random_eng, RoomType::Critical))
	{
		fprintf(stderr, "Couldn't generate a path with length %d", level_gen_conf.level_path_length);
		assert(0);
	}

	std::default_random_engine side_room_eng;
	side_room_eng.seed(level_gen_conf.seed);

	// use binomial distribution to shift number of side rooms generated for a specific room
	// we can at most generate 3 side rooms for a room as it needs to connect to the main path
	std::binomial_distribution num_side_rooms_dist(3, level_gen_conf.side_room_percentage / 10);

	// side room length follows normal distribution, i.e. larger side room density is, larger the side room length mean is
	std::normal_distribution<double> side_room_length_dist(level_gen_conf.side_room_percentage, 2.0);

	// generate side paths, iterate through all rooms, each room should only have one child after generating the main path
	PathNode * curr_room = starting_room;
	while (curr_room->children.size()) {
		assert(curr_room->children.size() == 1);
		PathNode * next_room = *(curr_room->children.begin());
		int num_side_rooms_to_generate = num_side_rooms_dist(side_room_eng);
		for (int i = 0; i < num_side_rooms_to_generate; i ++) {
			int room_length = side_room_length_dist(side_room_eng);
			room_length = (room_length < 0) ? 0 : (room_length > 9.95) ? 10 : room_length; 
			if (!generate_path_from_node(curr_room, room_length, visited_rooms, random_eng, RoomType::Side)) {
				// if we couldn't generate a side room of length, just generate one with length 1
				if (!generate_path_from_node(curr_room, 1, visited_rooms, random_eng, RoomType::Side)) {
					break;
				}
			}
		}
		curr_room = next_room;
	}

	// construct room generation engines
	RoomGenerationEngines room_rand_engines(level_gen_conf.seed);

	// prepare level snapshot
	rapidjson::Document level_snap_shot;
	level_snap_shot.SetObject();

	// TODO: replace after issue#110 is resolved
	// 4 and 5 are being hard-coded here, this relates to the room templates defined in generate_room, we would
	// want to handle this more appropriately when we have more room templates
	rapidjson::SetValueByPointer(
		level_snap_shot, rapidjson::Pointer("/player/position/x"), (starting_room->position % room_size) * room_size + 5);
	rapidjson::SetValueByPointer(
		level_snap_shot, rapidjson::Pointer("/player/position/y"), (starting_room->position / room_size) * room_size + 4);
	rapidjson::CreateValueByPointer(level_snap_shot, rapidjson::Pointer("/enemies/0"));

	// generate specific rooms and enemies
	traverse_path_and_generate_rooms(starting_room, level_gen_conf, level_conf, level_snap_shot, room_rand_engines);

	// update level snap shots
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	level_snap_shot.Accept(writer);
	level_conf.level_snap_shot = buffer.GetString();

	return level_conf;
}
