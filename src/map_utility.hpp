#pragma once
#include "components.hpp"

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

namespace MapUtility {
static constexpr uint8_t num_predefined_rooms = 8;
static constexpr uint8_t num_predefined_levels = 1;

// 10*10 grid used to represent map layout
using MapLayout = std::array<std::array<MapUtility::RoomID, MapUtility::room_size>, MapUtility::room_size>;
// room_layouts that contains generated rooms, each element is a 10*10 array that defines each tile textures in the
// 10*10 room. Note: we are saving uint32_t for tile ids, but tile_id is actually 8 bit, this is because opengl shaders
// only have 32 bit ints. However, number of tile textures are restricted in 8 bit integer( since the walkable/wall
// tiles set are uint8_t and our tile atlas only support 64 tiles), so it should always be safe to convert from uint32_t
// to uin8_t Predefined room ids: 0 -- void room
using RoomLayout = std::array<uint32_t, MapUtility::room_size * MapUtility::room_size>;

// The current level configurations
struct LevelConfiguration {
	// level snapshot that contains player and enemy information
	std::string level_snap_shot;
	// map layout of current level, 10*10 grid
	MapUtility::MapLayout map_layout;
	// room layout of current level, indexed by room ids
	std::vector<MapUtility::RoomLayout> room_layouts;
};

// Per-Level generation configuation, used as the metadata for generating a level,
// this is meant to be editable and serializable,
struct LevelGenConf {
	// The seed for generating pseudo random numbers, this is a constant so we generate the
	// same map every time.
	unsigned int seed = 10;
	// The number of rooms from start to end on a certain level
	unsigned int level_path_length = 6;

	// room-specific properties
	// decides how many solid tiles are spawned in a room
	double room_density = 0.2;
	// how many side rooms we have, 1 indicates there a side room next to each room
	double side_room_percentage = 0.2;
	// how complex the path in a room is
	double room_path_complexity = 0.5;
	// how dense traps are spawned
	double room_traps_density = 0.1;
	// determines how many iterations of cellular automata to run
	unsigned int room_smoothness = 0;

	// enemy properties
	double enemies_density = 0.5;

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

struct MapAreaIterator {
	using iterator_category = std::input_iterator_tag;
	using difference_type = vec2;
	using value_type = const uvec2;
	using pointer = value_type*;
	using reference = value_type&;

	reference operator*() const { return current_pos; }
	pointer operator->() { return &current_pos; }

	MapAreaIterator& operator++()
	{
		if (current_pos.x >= max_x) {
			current_pos.x = min_x;
			current_pos.y++;
		} else {
			current_pos.x++;
		}
		return *this;
	}

	MapAreaIterator operator++(int)
	{
		MapAreaIterator tmp = *this;
		++(*this);
		return tmp;
	}

	friend bool operator==(const MapAreaIterator& a, const MapAreaIterator& b)
	{
		return a.current_pos == b.current_pos;
	};
	friend bool operator!=(const MapAreaIterator& a, const MapAreaIterator& b)
	{
		return a.current_pos != b.current_pos;
	};

	MapAreaIterator(uint min_x, uint max_x, uvec2 current_pos);

private:
	uint min_x;
	uint max_x;
	uvec2 current_pos;
};

struct MapArea {

	MapArea(const MapPosition& map_pos, const MapHitbox& map_size);

	MapAreaIterator begin()
	{
		uvec2 current_pos;
		current_pos.x = (map_pos.position.x >= map_size.center.x) ? map_pos.position.x - map_size.center.x : 0;
		current_pos.y = (map_pos.position.y >= map_size.center.y) ? map_pos.position.y - map_size.center.y : 0;
		return MapAreaIterator(current_pos.x, current_pos.x + map_size.area.x - 1, current_pos);
	}
	MapAreaIterator end()
	{
		uvec2 current_pos;
		current_pos.x = (map_pos.position.x >= map_size.center.x) ? map_pos.position.x - map_size.center.x : 0;
		current_pos.y = (map_pos.position.y + map_size.area.y >= map_size.center.y)
			? map_pos.position.y + map_size.area.y - map_size.center.y
			: 0;
		return MapAreaIterator(current_pos.x - map_size.area.x + 1, current_pos.x, current_pos);
	}

private:
	const MapPosition& map_pos;
	const MapHitbox& map_size;
};
} // namespace MapUtility