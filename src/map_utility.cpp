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
MapUtility::MapArea::MapArea(const MapPosition& map_pos, const MapSize& map_size)
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
}
