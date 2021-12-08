#include "map_utility.hpp"

#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

static const rapidjson::Value* get_value_from_json(const std::string& prefix, const rapidjson::Document& json)
{
	const auto* value = rapidjson::GetValueByPointer(json, rapidjson::Pointer(prefix.c_str()));
	return value;
}

MapUtility::MapAreaIterator::MapAreaIterator(uint min_x, uint max_x, uvec2 current_pos)
	: min_x(min_x)
	, max_x(max_x)
	, current_pos(current_pos)
{
}
MapUtility::MapArea::MapArea(const MapPosition& map_pos, const MapHitbox& map_size)
	: map_pos(map_pos)
	, map_size(map_size)
{
}
void MapUtility::LevelGenConf::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/seed").c_str()), seed);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/level_path_length").c_str()), level_path_length);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/room_density").c_str()), room_density);
	rapidjson::SetValueByPointer(
		json, rapidjson::Pointer((prefix + "/side_room_percentage").c_str()), side_room_percentage);
	rapidjson::SetValueByPointer(
		json, rapidjson::Pointer((prefix + "/room_path_complexity").c_str()), room_path_complexity);
	rapidjson::SetValueByPointer(
		json, rapidjson::Pointer((prefix + "/room_traps_density").c_str()), room_traps_density);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/room_smoothness").c_str()), room_smoothness);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/enemies_density").c_str()), enemies_density);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/level_difficulty").c_str()), level_difficulty);
}

void MapUtility::LevelGenConf::deserialize(const std::string& prefix, const rapidjson::Document& json)
{
	if (const auto* seed_value = get_value_from_json(prefix + "/seed", json)) {
		seed = seed_value->GetUint();
	}
	if (const auto* level_path_length_value = get_value_from_json(prefix + "/level_path_length", json)) {
		level_path_length = level_path_length_value->GetUint();
	}
	if (const auto* room_density_value = get_value_from_json(prefix + "/room_density", json)) {
		room_density = room_density_value->GetDouble();
	}
	if (const auto* side_room_percentage_value = get_value_from_json(prefix + "/side_room_percentage", json)) {
		side_room_percentage = side_room_percentage_value->GetDouble();
	}
	if (const auto* room_path_complexity_value = get_value_from_json(prefix + "/room_path_complexity", json)) {
		room_path_complexity = room_path_complexity_value->GetDouble();
	}
	if (const auto* room_traps_density_value = get_value_from_json(prefix + "/room_traps_density", json)) {
		room_traps_density = room_traps_density_value->GetDouble();
	}
	if (const auto* room_smoothness_value = get_value_from_json(prefix + "/room_smoothness", json)) {
		room_smoothness = room_smoothness_value->GetUint();
	}
	if (const auto* enemies_density_value = get_value_from_json(prefix + "/enemies_density", json)) {
		enemies_density = enemies_density_value->GetDouble();
	}
	if (const auto* level_difficulty_value = get_value_from_json(prefix + "/level_difficulty", json)) {
		level_difficulty = level_difficulty_value->GetUint();
	}
}

const std::set<uint8_t>& MapUtility::floor_tiles()
{
	const static std::set<uint8_t> floor_tiles({ 0, 4, 5, 6, 7, 8, 16, 24, 32, 40, 52 });
	return floor_tiles;
}

bool MapUtility::is_trap_tile(TileID tile_id)
{
	// trap tiles are animated, 4 frames each, and occupies a rectangle on the sprite sheet
	const int trap_tile_start_id = 28;
	const int num_trap_tiles = 2;

	const int trap_tile_start_row = trap_tile_start_id / tile_sprite_sheet_size;
	const int trap_tile_start_col = trap_tile_start_id % tile_sprite_sheet_size;

	return ((trap_tile_start_row <= (tile_id / tile_sprite_sheet_size)
			 && (tile_id / tile_sprite_sheet_size) <= trap_tile_start_row + num_trap_tiles - 1)
			&& (trap_tile_start_col <= (tile_id % tile_sprite_sheet_size)
				&& (tile_id % tile_sprite_sheet_size) <= trap_tile_start_col + 4 - 1));
}

bool MapUtility::is_wall_tile(TileID tile_id)
{
	int tile_row = tile_id / tile_sprite_sheet_size;
	int tile_col = tile_id % tile_sprite_sheet_size;
	if (0 <= tile_row && tile_row < 4 && 1 <= tile_col && tile_col <= 3) {
		return true;
	}

	const static std::set<uint8_t> blocking_tiles({
		20,
		21,
		22,
		23, // torch
		44,
		45,
		46,
		47, // chest
		60,
		61,
		62, // door
		56,
		57,
		58, // cracked wall
	});

	return blocking_tiles.find(tile_id) != blocking_tiles.end();
}
