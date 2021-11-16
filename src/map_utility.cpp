#include "map_utility.hpp"

#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

static const rapidjson::Value * get_and_assert_value_from_json(const std::string& prefix, const rapidjson::Document& json) {
	const auto * value = rapidjson::GetValueByPointer(json, rapidjson::Pointer(prefix.c_str()));
	assert(value);
	return value;
}

void MapUtility::LevelGenConf::serialize(const std::string& prefix, rapidjson::Document& json) const {
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/seed").c_str()), seed);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/level_path_length").c_str()), level_path_length);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/room_density").c_str()), room_density);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/side_room_percentage").c_str()), side_room_percentage);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/room_path_complexity").c_str()), room_path_complexity);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/room_traps_density").c_str()), room_traps_density);
}

void MapUtility::LevelGenConf::deserialize(const std::string& prefix, const rapidjson::Document& json) {
	const auto* seed_value = get_and_assert_value_from_json(prefix + "/seed", json);
	seed = seed_value->GetUint();
	const auto* level_path_length_value = get_and_assert_value_from_json(prefix + "/level_path_length", json);
	level_path_length = level_path_length_value->GetUint();
	const auto* room_density_value = get_and_assert_value_from_json(prefix + "/room_density", json);
	room_density = room_density_value->GetDouble();
	const auto* side_room_percentage_value = get_and_assert_value_from_json(prefix + "/side_room_percentage", json);
	side_room_percentage = side_room_percentage_value->GetDouble();
	const auto* room_path_complexity_value = get_and_assert_value_from_json(prefix + "/room_path_complexity", json);
	room_path_complexity = room_path_complexity_value->GetDouble();
	const auto* room_traps_density_value = get_and_assert_value_from_json(prefix + "/room_traps_density", json);
	room_traps_density = room_traps_density_value->GetDouble();
}
