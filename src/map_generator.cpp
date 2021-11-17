#include "map_generator.hpp"

#include <set>

#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace MapUtility;

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

static bool is_straight_room(const std::set<Direction>& open_sides)
{
	if (open_sides.size() != 2) {
		return false;
	}
	return (opposite_direction(*(open_sides.begin())) == *(open_sides.end()));
}

RoomLayout MapGenerator::generate_room(const std::set<Direction>& open_directions,
									   RoomType room_type,
									   MapUtility::LevelGenConf level_gen_conf,
									   RoomGenerationEngines random_engs,
									   bool is_debugging)
{
	// Contants
	const static uint8_t solid_block_tile = 12;
	const static uint8_t void_tile = 10;
	const uint8_t tile_next_level = 14;
	const uint8_t tile_last_level = 20;
	const static std::array<uint8_t, 2> trap_tiles = { 30, 38 };
	// the entrance size on each open side
	const static int room_entrance_size = 2;
	// the start and end position of the entrance
	const static int room_entrance_start = (room_size - room_entrance_size) / 2;
	const static int room_entrance_end = (room_size + room_entrance_size) / 2 - 1;
	// max generation values
	const static double max_side_path_probability = 0.9;
	const static double max_traps_density = 0.3;

	// Room templates
	const static std::map<RoomType, RoomLayout> room_templates = {
		{ RoomType::Start,
		  {
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,12,12,12,0,0,0,
			0,0,0,0,12,20,12,0,0,0,
			0,0,0,0,12,0,12,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
		}},
		{ RoomType::End,
		  {
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,7,0,7,0,0,0,0,
			0,0,0,15,14,15,0,0,0,0,
			0,0,0,23,12,23,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
		}},
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

	std::uniform_int_distribution<int> corridar_width_dist(4, 8);
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
		bool asdad = is_on_entrance_path(room_tile_position);
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
		room_layout = room_templates.at(room_type);
	}

	if (is_debugging) {
		for (int room_loc : critical_locations) {
			room_layout.at(room_loc) = 10;
		}
	}

	std::bernoulli_distribution blocks_dist(level_gen_conf.room_density);
	std::bernoulli_distribution spawn_traps_dist(max_traps_density * level_gen_conf.room_traps_density);
	std::uniform_int_distribution<int> traps_dist(0, trap_tiles.size() - 1);
	for (int room_row = 0; room_row < room_size; room_row++) {
		for (int room_col = 0; room_col < room_size; room_col++) {
			int room_index = room_row * room_size + room_col;
			// also add entrance paths to critical locations
			if (is_on_entrance_path(room_index)) {
				critical_locations.emplace(room_index);
			}

			// if the tile is next level or last level tile already, we don't generate anything
			if (room_layout.at(room_index) == tile_next_level || room_layout.at(room_index) == tile_last_level) {
				continue;
			}

			if (is_outside_tile(room_index)) {
				room_layout.at(room_index) = void_tile;
			} else if (is_boundary_tile(room_index)) {
				room_layout.at(room_index) = solid_block_tile;
			} else {
				// // spin the random generator every time to keep random blocks independent to traps
				// bool will_try_spawn_block = ;
				if (spawn_traps_dist(random_engs.traps_eng)) {
					room_layout.at(room_index) = trap_tiles.at(traps_dist(random_engs.general_eng));
				} else if ((room_type == RoomType::Critical)
						   && (critical_locations.find(room_index) == critical_locations.end())
						   && blocks_dist(random_engs.general_eng)) {
					room_layout.at(room_index) = solid_block_tile;
				}
			}
		}
	}

	return room_layout;
}

LevelConfiguration MapGenerator::generate_level(LevelGenConf level_gen_conf, bool is_debugging)
{
	LevelConfiguration level_conf;
	// prepare the random engines
	std::default_random_engine random_eng;
	random_eng.seed(level_gen_conf.seed);

	// initialize all rooms to be void first
	RoomLayout void_room;
	void_room.fill(10);
	level_conf.room_layouts.emplace_back(void_room);
	for (int row = 0; row < map_size; row++) {
		for (int col = 0; col < map_size; col++) {
			level_conf.map_layout.at(row).at(col) = 0;
		}
	}

	// 1. Start procedural generation by choose a random position to place the first room
	std::uniform_int_distribution<int> random_number_distribution(0, map_size - 1);
	int start_row = random_number_distribution(random_eng);
	int start_col = random_number_distribution(random_eng);

	// use an array to keep the path, each element is calculated by 10 * row + col
	std::vector<int> path = { start_row * map_size + start_col };
	// Keeps a direction_tried set for each room, so we could backtrack
	std::vector<std::set<Direction>> directions_tried(1);
	int current_row = start_row;
	int current_col = start_col;
	int current_length = 1;

	// randomly choose one of the four directions
	std::uniform_int_distribution<int> direction_rand(0, 3);

	while (current_length < level_gen_conf.level_path_length) {
		// 2. Explore a random direction that we haven't tried before, if we consumed all directions,
		// backtrack to the last cell and continue this process
		auto& current_directions_tried = directions_tried.back();
		while (current_directions_tried.size() < 4) {
			Direction direction = static_cast<Direction>(direction_rand(random_eng));

			// try a direction we haven't tried before
			if (current_directions_tried.find(direction) != current_directions_tried.end()) {
				continue;
			}
			current_directions_tried.emplace(direction);

			int next_row = current_row + direction_vec.at(direction).at(1);
			int next_col = current_col + direction_vec.at(direction).at(0);
			// make sure we are not out of bounds
			if (next_row >= map_size || next_row < 0 || next_col >= map_size || next_col < 0) {
				continue;
			}

			// make sure the room has not been added to path
			if (std::find(path.begin(), path.end(), (next_row * map_size + next_col)) != path.end()) {
				continue;
			}

			path.emplace_back(next_row * map_size + next_col);
			// the direction tried for the generated room will be the opposite of currently chosen direction
			directions_tried.emplace_back(std::set<Direction>({ opposite_direction(direction) }));
			current_row = next_row;
			current_col = next_col;
			current_length++;
			break;
		}

		// backtrack if we have checked all four directions but cannot find a valid path
		if (directions_tried.back().size() >= 4) {
			path.pop_back();
			directions_tried.pop_back();
			current_row = path.back() / map_size;
			current_col = path.back() % map_size;
			current_length--;
		}

		if (current_length < 0) {
			// we tried all possibility, but still cannot find a valid path
			fprintf(stderr, "Couldn't generate a path with length %d", level_gen_conf.level_path_length);
			assert(0);
		}
	}

	// get the open direction of the room given the start room to end room, note the two rooms are expect to be
	// neighbour
	auto get_open_direction = [](int from_room_index, int to_room_index) {
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

	// check if the room to specified direction will exceed the map boundaries
	auto will_exceed_boundary = [](int room_position, Direction direction) {
		return ((room_position / room_size == 0 && direction == Direction::Up)
				|| (room_position / room_size == room_size - 1 && direction == Direction::Down)
				|| (room_position % room_size == 0 && direction == Direction::Left)
				|| (room_position % room_size == room_size - 1 && direction == Direction::Right));
	};

	// Construct room generation engines
	RoomGenerationEngines room_rand_engines(level_gen_conf.seed);
	// prepare level snapshot
	rapidjson::Document level_snap_shot;
	rapidjson::CreateValueByPointer(level_snap_shot, rapidjson::Pointer("/enemies/0"));

	// side room distribution based on side room percentage
	std::bernoulli_distribution side_room_dist(level_gen_conf.side_room_percentage);

	for (int i = 0; i < path.size(); i++) {
		std::set<Direction> open_directions;
		int room_position = path.at(i);
		int row = room_position / 10;
		int col = room_position % 10;

		// add open directions based on last and next rooms
		if (i - 1 >= 0) {
			open_directions.emplace(get_open_direction(room_position, path.at(i - 1)));
		}
		if (i + 1 < path.size()) {
			open_directions.emplace(get_open_direction(room_position, path.at(i + 1)));
		}

		// determine if we are generating a side room
		if (side_room_dist(random_eng)) {
			// iterate through all directions and see if we can find one available
			for (uint8_t d = 0; d < 4; d++) {
				Direction direction = static_cast<Direction>(d);
				if (will_exceed_boundary(room_position, direction)) {
					continue;
				}
				int target_position = room_position + 10 * direction_vec.at(static_cast<Direction>(d)).at(1)
					+ direction_vec.at(static_cast<Direction>(d)).at(0);
				if (target_position >= 0 && target_position < room_size * room_size
					&& std::find(path.begin(), path.end(), target_position) == path.end()) {
					level_conf.room_layouts.emplace_back(
						generate_room({ get_open_direction(target_position, room_position) },
									  RoomType::Side,
									  level_gen_conf,
									  room_rand_engines,
									  is_debugging));
					level_conf.map_layout.at(target_position / 10).at(target_position % 10)
						= static_cast<RoomID>(level_conf.room_layouts.size() - 1);

					// add side room direction to open directions
					open_directions.emplace(get_open_direction(room_position, target_position));
					break;
				}
			}
		}

		RoomLayout room_layout;
		if (i == 0) {
			// generating start room
			room_layout
				= generate_room(open_directions, RoomType::Start, level_gen_conf, room_rand_engines, is_debugging);
			// TODO: replace after issue#110 is resolved
			// 4 and 5 are being hard-coded here, this relates to the room templates defined in generate_room, we would
			// want to handle this more appropriately when we have more room templates
			rapidjson::SetValueByPointer(
				level_snap_shot, rapidjson::Pointer("/player/position/x"), col * room_size + 5);
			rapidjson::SetValueByPointer(
				level_snap_shot, rapidjson::Pointer("/player/position/y"), row * room_size + 5);
		} else {
			// generate a room on the critical path
			room_layout = generate_room(open_directions,
										(i == path.size() - 1) ? RoomType::End : RoomType::Critical,
										level_gen_conf,
										room_rand_engines,
										is_debugging);
		}

		level_conf.room_layouts.emplace_back(room_layout);
		level_conf.map_layout.at(row).at(col) = static_cast<RoomID>(level_conf.room_layouts.size() - 1);
	}

	// update level snap shots
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	level_snap_shot.Accept(writer);
	level_conf.level_snap_shot = buffer.GetString();

	return level_conf;
}
