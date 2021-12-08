#include "components.hpp"
#include "render_system.hpp" // for gl_has_errors

#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb_image/stb_image.h"

// stlib
#include <iostream>
#include <sstream>

#include <glm/gtx/rotate_vector.hpp>

#include "rapidjson/pointer.h"

// Very, VERY simple OBJ loader from https://github.com/opengl-tutorials/ogl tutorial 7
// (modified to also read vertex color and omit uv and normals)
bool Mesh::load_from_obj_file(const std::string& obj_path,
							  std::vector<ColoredVertex>& out_vertices,
							  std::vector<uint16_t>& out_vertex_indices,
							  vec2& out_size)
{
	// disable warnings about fscanf and fopen on Windows
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

	printf("Loading OBJ file %s...\n", obj_path.c_str());
	// Note, normal and UV indices are currently not used
	std::vector<uint16_t> out_uv_indices, out_normal_indices;
	std::vector<glm::vec2> out_uvs;
	std::vector<glm::vec3> out_normals;

	FILE* file = fopen(obj_path.c_str(), "r");
	if (file == nullptr) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (true) {
		std::array<char, 128> line_header = {};
		// read the first word of the line
		int res = fscanf(file, "%s", line_header.data());
		if (res == EOF) {
			break; // EOF = End Of File. Quit the loop.
		}

		if (strcmp((char*)line_header.data(), "v") == 0) {
			ColoredVertex vertex;
			fscanf(file,
				   "%f %f %f %f %f %f\n",
				   &vertex.position.x,
				   &vertex.position.y,
				   &vertex.position.z,
				   &vertex.color.x,
				   &vertex.color.y,
				   &vertex.color.z);
			out_vertices.push_back(vertex);
		} else if (strcmp((char*)line_header.data(), "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you
						  // want to use TGA or BMP loaders.
			out_uvs.push_back(uv);
		} else if (strcmp((char*)line_header.data(), "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			out_normals.push_back(normal);
		} else if (strcmp((char*)line_header.data(), "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			uvec3 vertex_index, normal_index; // , uvIndex[3]
			// int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertex_index[0], &uvIndex[0],
			// &normal_index[0], &vertex_index[1], &uvIndex[1], &normal_index[1], &vertex_index[2], &uvIndex[2],
			// &normal_index[2]);
			int matches = fscanf(file,
								 "%d//%d %d//%d %d//%d\n",
								 &vertex_index[0],
								 &normal_index[0],
								 &vertex_index[1],
								 &normal_index[1],
								 &vertex_index[2],
								 &normal_index[2]);
			if (matches != 6) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			// -1 since .obj starts counting at 1 and OpenGL starts at 0
			out_vertex_indices.push_back((uint16_t)vertex_index[0] - 1);
			out_vertex_indices.push_back((uint16_t)vertex_index[1] - 1);
			out_vertex_indices.push_back((uint16_t)vertex_index[2] - 1);
			// out_uv_indices.push_back(uvIndex[0] - 1);
			// out_uv_indices.push_back(uvIndex[1] - 1);
			// out_uv_indices.push_back(uvIndex[2] - 1);
			out_normal_indices.push_back((uint16_t)normal_index[0] - 1);
			out_normal_indices.push_back((uint16_t)normal_index[1] - 1);
			out_normal_indices.push_back((uint16_t)normal_index[2] - 1);
		} else {
			// Probably a comment, eat up the rest of the line
			std::array<char, 1000> stupid_buffer = {};
			fgets(stupid_buffer.data(), 1000, file);
		}
	}
	fclose(file);

	// Compute bounds of the mesh
	vec3 max_position = { -99999, -99999, -99999 };
	vec3 min_position = { 99999, 99999, 99999 };
	for (ColoredVertex& pos : out_vertices) {
		max_position = glm::max(max_position, pos.position);
		min_position = glm::min(min_position, pos.position);
	}
	min_position.z = 0; // don't scale z direction
	max_position.z = 1;
	vec3 size3d = max_position - min_position;
	out_size = size3d;

	// Normalize mesh to range -0.5 ... 0.5
	for (ColoredVertex& pos : out_vertices) {
		pos.position = ((pos.position - min_position) / size3d) - vec3(0.5f, 0.5f, 0.f);
	}

	return true;
}

static const rapidjson::Value* get_and_assert_value_from_json(const std::string& prefix,
															  const rapidjson::Document& json)
{
	const auto* value = rapidjson::GetValueByPointer(json, rapidjson::Pointer(prefix.c_str()));
	assert(value);
	return value;
}

void MapPosition::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/position/x").c_str()), position.x);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/position/y").c_str()), position.y);
}

void MapPosition::deserialize(Entity entity, const std::string& prefix, const rapidjson::Document& json)
{
	const auto* position_x = get_and_assert_value_from_json(prefix + "/position/x", json);
	position.x = position_x->GetInt();
	const auto* position_y = get_and_assert_value_from_json(prefix + "/position/y", json);
	position.y = position_y->GetInt();
	const auto* location = get_and_assert_value_from_json(prefix, json);
	if (location->HasMember("tile_area") && location->HasMember("tile_center")) {
		uvec2 area = { get_and_assert_value_from_json(prefix + "/tile_area/0", json)->GetUint(),
					   get_and_assert_value_from_json(prefix + "/tile_area/1", json)->GetUint() };
		uvec2 center = { get_and_assert_value_from_json(prefix + "/tile_center/0", json)->GetUint(),
						 get_and_assert_value_from_json(prefix + "/tile_center/1", json)->GetUint() };
		registry.emplace<MapHitbox>(entity, area, center);
	}
}

void Enemy::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/team").c_str()), static_cast<int>(team));
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/type").c_str()), static_cast<int>(type));
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/danger_rating").c_str()), danger_rating);
	if (loot_multiplier != 1) {
		rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/loot_multiplier").c_str()), loot_multiplier);
	}
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/state").c_str()), static_cast<int>(state));
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/radius").c_str()), radius);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/speed").c_str()), speed);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/attack_range").c_str()), attack_range);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/nest_position/x").c_str()), nest_map_pos.x);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/nest_position/y").c_str()), nest_map_pos.y);
}

void Enemy::deserialize(const std::string& prefix, const rapidjson::Document& json, bool load_from_file)
{
	// Enemy Type
	const auto* type_value = get_and_assert_value_from_json(prefix + "/type", json);
	type = static_cast<EnemyType>(type_value->GetInt());

	// Danger Rating
	const auto* danger_rating_value = get_and_assert_value_from_json(prefix + "/danger_rating", json);
	danger_rating = danger_rating_value->GetUint();
	assert(danger_rating <= max_danger_rating);

	// Loot Multiplier (Optional)
	const auto* loot_multiplier_value = rapidjson::GetValueByPointer(json, rapidjson::Pointer((prefix + "/loot_multiplier").c_str()));
	if (loot_multiplier_value != nullptr) {
		loot_multiplier = loot_multiplier_value->GetUint();
	}

	// State
	const auto* state_value = get_and_assert_value_from_json(prefix + "/state", json);
	state = static_cast<EnemyState>(state_value->GetInt());

	// Detection Radius
	const auto* radius_value = get_and_assert_value_from_json(prefix + "/radius", json);
	radius = radius_value->GetInt();

	// Movement Speed
	const auto* speed_value = get_and_assert_value_from_json(prefix + "/speed", json);
	speed = speed_value->GetInt();

	// Attack Range
	const auto* attack_range_value = get_and_assert_value_from_json(prefix + "/attack_range", json);
	attack_range = attack_range_value->GetInt();
	if (load_from_file) {
		// Team
		const auto* team_value = get_and_assert_value_from_json(prefix + "/team", json);
		team = static_cast<ColorState>(team_value->GetInt());
		// Nest Position
		const auto* nest_pos_x = get_and_assert_value_from_json(prefix + "/nest_position/x", json);
		nest_map_pos.x = nest_pos_x->GetInt();
		const auto* nest_pos_y = get_and_assert_value_from_json(prefix + "/nest_position/y", json);
		nest_map_pos.y = nest_pos_y->GetInt();
	}
}

bool Attack::can_reach(Entity attacker, uvec2 target) const
{
	if (targeting_type == TargetingType::Adjacent) {
		ivec2 distance_vec = abs(ivec2(target - registry.get<MapPosition>(attacker).position));
		int distance = abs(distance_vec.x - distance_vec.y) + min(distance_vec.x, distance_vec.y) * 3 / 2;
		if (distance > range) {
			return false;
		}
	}
	return true;
}

bool Attack::is_in_range(uvec2 source, uvec2 target, uvec2 pos) const
{
	dvec2 area = dvec2(parallel_size - 1, perpendicular_size - 1);
	dvec2 distance = dvec2(target) - dvec2(pos);
	dvec2 attack_direction = dvec2(target) - dvec2(source);
	double attack_angle = atan2(attack_direction.y, attack_direction.x);
	dvec2 aligned_distance = round(abs(glm::rotate(distance, -attack_angle)));
	if (pattern == AttackPattern::Rectangle) {
		return abs(aligned_distance.x) <= area.x && abs(aligned_distance.y) <= area.y;
	}
	if (pattern == AttackPattern::Circle) {
		auto make_square = [](double dist, double max) {
			if (max == 0) {
				return dist == 0 ? 0.0 : 2.0;
			}
			return pow(dist / max, 2);
		};
		dvec2 dist_squared = { make_square(aligned_distance.x, area.x), make_square(aligned_distance.y, area.y) };
		return dist_squared.x + dist_squared.y <= 1;
	}
	return true;
}

void Attack::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/name").c_str()), name.c_str());
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/to_hit_min").c_str()), to_hit_min);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/to_hit_max").c_str()), to_hit_max);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/damage_min").c_str()), damage_min);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/damage_max").c_str()), damage_max);
	rapidjson::SetValueByPointer(
		json, rapidjson::Pointer((prefix + "/damage_type").c_str()), static_cast<int>(damage_type));
	rapidjson::SetValueByPointer(
		json, rapidjson::Pointer((prefix + "/targeting_type").c_str()), static_cast<int>(targeting_type));
}

void Attack::deserialize(const std::string& prefix, const rapidjson::Document& json)
{
	const auto* name_value = get_and_assert_value_from_json(prefix + "/name", json);
	name = name_value->GetString();
	const auto* to_hit_min_value = get_and_assert_value_from_json(prefix + "/to_hit_min", json);
	to_hit_min = to_hit_min_value->GetInt();
	const auto* to_hit_max_value = get_and_assert_value_from_json(prefix + "/to_hit_max", json);
	to_hit_max = to_hit_max_value->GetInt();
	const auto* damage_min_value = get_and_assert_value_from_json(prefix + "/damage_min", json);
	damage_min = damage_min_value->GetInt();
	const auto* damage_max_value = get_and_assert_value_from_json(prefix + "/damage_max", json);
	damage_max = damage_max_value->GetInt();
	const auto* damage_type_value = get_and_assert_value_from_json(prefix + "/damage_type", json);
	damage_type = static_cast<DamageType>(damage_type_value->GetInt());
	const auto* targeting_type_value = get_and_assert_value_from_json(prefix + "/targeting_type", json);
	targeting_type = static_cast<TargetingType>(targeting_type_value->GetInt());
}

void Attack::deserialize(const rapidjson::GenericObject<false, rapidjson::Value>& attack_json)
{
	name = attack_json["name"].GetString();
	targeting_type = (strcmp(attack_json["targeting_type"].GetString(), "Adjacent") == 0) ? TargetingType::Adjacent
																						  : TargetingType::Projectile;
	if (attack_json.HasMember("mana_cost")) {
		mana_cost = attack_json["mana_cost"].GetInt();
	}
	if (attack_json.HasMember("cost")) {
		turn_cost = attack_json["cost"].GetInt();
	}
	if (attack_json.HasMember("range")) {
		range = attack_json["range"].GetInt();
	}
	if (attack_json.HasMember("pattern")) {
		pattern = (strcmp(attack_json["pattern"].GetString(), "Rectangle") == 0) ? AttackPattern::Rectangle
																						: AttackPattern::Circle;
	}
	if (attack_json.HasMember("parallel_size")) {
		parallel_size = attack_json["parallel_size"].GetInt();
	}
	if (attack_json.HasMember("perpendicular_size")) {
		perpendicular_size = attack_json["perpendicular_size"].GetInt();
	}
	to_hit_min = attack_json["to_hit"][0].GetInt();
	to_hit_max = attack_json["to_hit"][1].GetInt();
	damage_min = attack_json["damage"][0].GetInt();
	damage_max = attack_json["damage"][1].GetInt();
	auto damage_type_json
		= std::find(damage_type_names.begin(), damage_type_names.end(), attack_json["damage_type"].GetString());
	if (damage_type_json != damage_type_names.end()) {
		damage_type = (DamageType)(damage_type_json - damage_type_names.begin());
	}
	if (attack_json.HasMember("effects")) {
		for (rapidjson::SizeType i = 0; i < attack_json["effects"].Size(); i++) {
			auto effect_json = attack_json["effects"][i].GetObj();
			Entity effect_entity = registry.create();
			EffectEntry& effect_entry = registry.emplace<EffectEntry>(effect_entity);
			effect_entry.next_effect = effects;
			effects = effect_entity;
			const char* json_effect_name = effect_json["effect"].GetString();
			for (size_t k = 0; k < effect_names.size(); k++) {
				const auto& effect_name = effect_names.at(k);
				if (effect_name.compare(json_effect_name) == 0) {
					effect_entry.effect = (Effect)k;
				}
			}
			effect_entry.chance = effect_json["chance"].GetFloat();
			effect_entry.magnitude = effect_json["magnitude"].GetInt();
		}
	}
}

void Stats::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/health").c_str()), health);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/health_max").c_str()), health_max);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/mana").c_str()), mana);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/mana_max").c_str()), mana_max);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/to_hit_bonus/Weapons").c_str()), to_hit_weapons);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/to_hit_bonus/Spells").c_str()), to_hit_spells);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/evasion").c_str()), evasion);

	base_attack.serialize(prefix + "/attack", json);
	for (size_t type = 0; type < (size_t)DamageType::Count; type++) {
		int bonus = damage_bonus.at(type);
		if (bonus != 0) {
			std::string path = prefix + "/damage_bonus/";
			path += damage_type_names.at(type);
			rapidjson::SetValueByPointer(json, rapidjson::Pointer(path.c_str()), bonus);
		}
		int mod = damage_modifiers.at(type);
		if (mod != 0) {
			std::string path = prefix + "/damage_mods/";
			path += damage_type_names.at(type);
			rapidjson::SetValueByPointer(json, rapidjson::Pointer(path.c_str()), mod);
		}
	}
}

void Stats::deserialize(const std::string& prefix, const rapidjson::Document& json)
{
	const auto* health_value = get_and_assert_value_from_json(prefix + "/health", json);
	health = health_value->GetInt();
	const auto* health_max_value = get_and_assert_value_from_json(prefix + "/health_max", json);
	health_max = health_max_value->GetInt();
	const auto* mana_value = get_and_assert_value_from_json(prefix + "/mana", json);
	mana = mana_value->GetInt();
	const auto* mana_max_value = get_and_assert_value_from_json(prefix + "/mana_max", json);
	mana_max = mana_max_value->GetInt();
	const auto* to_hit_bonus_value = get_and_assert_value_from_json(prefix + "/to_hit_bonus", json);
	if (to_hit_bonus_value->IsInt()) {
		to_hit_weapons = to_hit_bonus_value->GetInt();
		to_hit_spells = to_hit_bonus_value->GetInt();
	} else {
		to_hit_weapons = to_hit_bonus_value->HasMember("Weapons") ? (*to_hit_bonus_value)["Weapons"].GetInt() : 0;
		to_hit_spells = to_hit_bonus_value->HasMember("Spells") ? (*to_hit_bonus_value)["Spells"].GetInt() : 0;
	}
	const auto* damage_bonus_value = get_and_assert_value_from_json(prefix + "/damage_bonus", json);
	if (damage_bonus_value->IsInt()) {
		damage_bonus.fill(damage_bonus_value->GetInt());
	}
	const auto* evasion_value = get_and_assert_value_from_json(prefix + "/evasion", json);
	evasion = evasion_value->GetInt();
	base_attack.deserialize(prefix + "/attack", json);
	for (size_t type = 0; type < (size_t)DamageType::Count; type++) {
		std::string path = prefix + "/damage_bonus/";
		path += damage_type_names.at(type);
		const auto* bonus = rapidjson::GetValueByPointer(json, rapidjson::Pointer(path.c_str()));
		if (bonus != nullptr && bonus->IsInt()) {
			damage_bonus.at(type) = bonus->GetInt();
		}

		path = prefix + "/damage_mods/";
		path += damage_type_names.at(type);
		const auto* mod = rapidjson::GetValueByPointer(json, rapidjson::Pointer(path.c_str()));
		if (mod != nullptr && mod->IsInt()) {
			damage_modifiers.at(type) = mod->GetInt();
		}
	}
}

void Item::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	// note: this can cause problem depending on entt's implementations, but since item templates are created at the beginning, it should be relatively safe
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/item_template").c_str()), static_cast<uint32_t>(item_template));
}
void Item::deserialize(const std::string& prefix, const rapidjson::Document& json)
{
	const auto* item_template_value = get_and_assert_value_from_json(prefix + "/item_template", json);
	item_template = static_cast<Entity>(item_template_value->GetUint());
}

void ResourcePickup::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/resource").c_str()), static_cast<int>(resource));
}
void ResourcePickup::deserialize(const std::string& prefix, const rapidjson::Document& json)
{
	const auto* resource_value = get_and_assert_value_from_json(prefix + "/resource", json);
	resource = static_cast<Resource>(resource_value->GetInt());
}

bool operator==(const Text& t1, const Text& t2) { return t1.text == t2.text && t1.font_size == t2.font_size; }

void Collision::add(Entity parent, Entity child)
{
	Collision* collision = registry.try_get<Collision>(parent);
	if (collision == nullptr) {
		collision = &(registry.emplace<Collision>(parent, registry.create()));
		registry.emplace<CollisionEntry>(collision->children, parent, entt::null, child);
	} else {
		Entity new_collision = registry.create();
		registry.emplace<CollisionEntry>(new_collision, parent, collision->children, child);
		collision->children = new_collision;
	}
}

void UIGroup::add_element(Entity group, Entity element, UIElement& ui_element, UILayer layer)
{
	if (group == entt::null) {
		return;
	}
	UIGroup& g = registry.get<UIGroup>(group);
	ui_element.next = g.first_elements.at((size_t)layer);
	g.first_elements.at((size_t)layer) = element;
}

void UIGroup::remove_element(Entity group, Entity element, UILayer layer)
{
	if (group == entt::null) {
		return;
	}
	UIGroup& g = registry.get<UIGroup>(group);
	Entity prev = g.first_elements.at((size_t)layer);
	Entity curr = registry.get<UIElement>(prev).next;
	if (prev == element) {
		g.first_elements.at((size_t)layer) = registry.get<UIElement>(prev).next;
		return;
	}
	while (curr != entt::null) {
		if (curr == element) {
			registry.get<UIElement>(prev).next = registry.get<UIElement>(curr).next;
			return;
		}
		UIElement& element = registry.get<UIElement>(curr);
		prev = curr;
		curr = element.next;
	}
}

Entity UIGroup::find(Groups group)
{
	for (auto [entity, group_component] : registry.view<UIGroup>().each()) {
		if (group_component.identifier == group) {
			return entity;
		}
	}
	return entt::null;
}

Entity Inventory::get(Entity entity, Slot slot) { return registry.get<Inventory>(entity).equipped.at((size_t)slot); }

void ItemTemplate::deserialize(Entity entity, const rapidjson::GenericObject<false, rapidjson::Value>& item)
{
	name = item["name"].GetString();
	tier = item["tier"].GetInt();

	// Slots
	for (rapidjson::SizeType j = 0; j < item["slots"].Size(); j++) {
		const char* json_slot_name = item["slots"][j].GetString();
		for (int k = 0; k < slot_names.size(); k++) {
			const auto& slot_name = slot_names.at(k);
			if (slot_name.compare(json_slot_name) == 0) {
				allowed_slots[k] = true;
			}
		}
	}

	// Attacks
	if (item.HasMember("attacks")) {
		Weapon& weapon = registry.emplace<Weapon>(entity);
		for (rapidjson::SizeType j = 0; j < item["attacks"].Size(); j++) {
			Entity attack_entity = registry.create();
			Attack& attack = registry.emplace<Attack>(attack_entity, "");
			attack.deserialize(item["attacks"][j].GetObj());
			weapon.given_attacks.emplace_back(attack_entity);
		}
	}

	if (item.HasMember("stat_boosts")) {
		StatBoosts& stat_boosts = registry.emplace<StatBoosts>(entity);
		stat_boosts.deserialize(item["stat_boosts"].GetObj());
	}

	if (item.HasMember("texture_offset")) {
		texture_offset = vec2(item["texture_offset"][0].GetInt(), item["texture_offset"][1].GetInt());
		if (item.HasMember("texture_size")) {
			texture_size = vec2(item["texture_size"][0].GetInt(), item["texture_size"][1].GetInt());
		}
	}
}

void StatBoosts::deserialize(const rapidjson::GenericObject<false, rapidjson::Value>& boosts) {
	if (boosts.HasMember("health")) {
		health = boosts["health"].GetInt();
	}
	if (boosts.HasMember("mana")) {
		mana = boosts["mana"].GetInt();
	}
	if (boosts.HasMember("luck")) {
		luck = boosts["luck"].GetInt();
	}
	if (boosts.HasMember("light")) {
		light = boosts["light"].GetInt();
	}
	if (boosts.HasMember("to_hit")) {
		if (boosts["to_hit"].HasMember("Weapons")) {
			to_hit_weapons = boosts["to_hit"]["Weapons"].GetInt();
		}
		if (boosts["to_hit"].HasMember("Spells")) {
			to_hit_spells = boosts["to_hit"]["Spells"].GetInt();
		}
	}
	if (boosts.HasMember("damage")) {
		auto damage_json = boosts["damage"].GetObj();
		for (size_t i = 0; i < damage_type_names.size(); i++) {
			if (damage_json.HasMember(damage_type_names.at(i).data())) {
				damage_bonus.at(i) = damage_json[damage_type_names.at(i).data()].GetInt();
			}
		}
	}
	if (boosts.HasMember("evasion")) {
		evasion = boosts["evasion"].GetInt();
	}
	if (boosts.HasMember("damage_mods")) {
		auto damage_mods_json = boosts["damage_mods"].GetObj();
		for (size_t i = 0; i < damage_type_names.size(); i++) {
			if (damage_mods_json.HasMember(damage_type_names.at(i).data())) {
				damage_modifiers.at(i) = damage_mods_json[damage_type_names.at(i).data()].GetInt();
			}
		}
	}
}

std::string Item::get_description(bool detailed) const
{
	ItemTemplate& item = registry.get<ItemTemplate>(item_template);
	std::string description = item.name;

	description += " - Tier " + std::to_string(item.tier);

	if (!detailed) {
		return description;
	}
	
	if (StatBoosts* boosts = registry.try_get<StatBoosts>(item_template)) {
		description += boosts->get_description();
	}

	if (Weapon* weapon = registry.try_get<Weapon>(item_template)) {
		description += weapon->get_description();
	}

	return description;
}

std::string int_to_signed_string(int i) {return ((i >= 0) ? '+' : '-') + std::to_string(abs(i)); }

std::string StatBoosts::get_description() const
{
	std::string description;
	if (health != 0) {
		description += "\n" + int_to_signed_string(health) + " health";
	}
	if (mana != 0) {
		description += "\n" + int_to_signed_string(mana) + " mana";
	}
	if (luck != 0) {
		description += "\n" + int_to_signed_string(luck) + " luck";
	}
	if (to_hit_weapons != 0) {
		description += "\n" + int_to_signed_string(to_hit_weapons) + " to weapon hit";
	}
	if (to_hit_spells != 0) {
		description += "\n" + int_to_signed_string(to_hit_spells) + " to spell hit";
	}
	for (size_t i = 0; i < (size_t)DamageType::Count; i++) {
		if (damage_bonus.at(i) == 0) {
			continue;
		}
		description
			+= "\n" + int_to_signed_string(damage_bonus.at(i)) + " ";
		description += damage_type_names.at(i);
		description += " dmg";
	}
	if (evasion != 0) {
		description += "\n" + int_to_signed_string(evasion) + " evasion";
	}
	for (size_t i = 0; i < (size_t)DamageType::Count; i++) {
		int mod = damage_modifiers.at(i);
		if (mod != 0) {
			description += "\n" + std::to_string(abs(mod)) + " ";
			description += damage_type_names.at(i);
			description += (mod < 0) ? " resistance" : " vulnerability";
		}
	}
	return description;
}

void StatBoosts::apply(Entity boosts, Entity target, bool applying)
{
	if (boosts == entt::null) {
		return;
	}
	StatBoosts* stat_boosts = registry.try_get<StatBoosts>(boosts);
	if (stat_boosts == nullptr) {
		return;
	}
	Stats& stats = registry.get<Stats>(target);
	int applying_mult = (applying) ? 1 : -1;
	stats.health_max += stat_boosts->health * applying_mult;
	stats.mana_max += stat_boosts->mana * applying_mult;
	stats.to_hit_weapons += stat_boosts->to_hit_weapons * applying_mult;
	stats.to_hit_spells += stat_boosts->to_hit_spells * applying_mult;
	stats.evasion += stat_boosts->evasion * applying_mult;
	for (size_t i = 0; i < stats.damage_modifiers.size(); i++) {
		stats.damage_bonus.at(i) += stat_boosts->damage_bonus.at(i) * applying_mult;
		stats.damage_modifiers.at(i) += stat_boosts->damage_modifiers.at(i) * applying_mult;
	}
	registry.get<PlayerStats>(target).luck += stat_boosts->luck * applying_mult;
	registry.get<Light>(target).radius += static_cast<float>(stat_boosts->light * applying_mult) * MapUtility::tile_size;
}

std::string Weapon::get_description() {
	std::string description = "\n-Attacks-";
	for (auto attack_entity : given_attacks) {
		description += "\n" + registry.get<Attack>(attack_entity).get_description();
	}
	return description;
}

std::string Attack::get_description() const {
	using std::to_string;
	std::string description = name + "\n  ";
	if (mana_cost != 0) {
		description += to_string(mana_cost) + " mana\n  ";
	}
	if (turn_cost > 1) {
		description += to_string(turn_cost) + " turns\n  ";
	}

	// To hit
	description += to_string(to_hit_min) + "-" + to_string(to_hit_max) + " to hit\n  ";

	// Damage
	description += to_string(damage_min) + "-" + to_string(damage_max) + " ";
	description += damage_type_names.at((size_t)damage_type);
	description += " dmg\n  ";

	if (targeting_type == TargetingType::Adjacent) {
		if (range > 1) {
			description += "range " + to_string(range) + "\n  ";
		}
		if (perpendicular_size > 1 || parallel_size > 1) {
			description += to_string(parallel_size) + "x" + to_string(perpendicular_size) + " area\n  ";
		}
	} else {
		description += "projectile\n  ";
	}

	Entity curr = effects;
	while (curr != entt::null) {
		EffectEntry& effect = registry.get<EffectEntry>(curr);
		description += to_string(static_cast<int>(effect.chance * 100.f)) + "% ";
		description += effect_names.at((size_t)effect.effect);
		description += " " + to_string(effect.magnitude) + "\n  ";
		curr = effect.next_effect;
	}

	// Remove trailing "\n  "
	description.pop_back();
	description.pop_back();
	description.pop_back();

	return description;
}

Entity AOESource::add(Entity parent)
{
	AOESource* source = registry.try_get<AOESource>(parent);
	Entity new_aoe = registry.create();
	if (source == nullptr) {
		registry.emplace<AOESource>(parent, new_aoe);
		registry.emplace<AOESquare>(new_aoe, parent, entt::null);
	} else {
		registry.emplace<AOESquare>(new_aoe, parent, source->children);
		source->children = new_aoe;
	}
	return new_aoe;
}

void BigRoom::add_room(Entity big_room, Entity room)
{
	BigRoom& big_room_component = registry.get_or_emplace<BigRoom>(big_room);
	BigRoomElement& element = registry.get_or_emplace<BigRoomElement>(room);
	element.big_room = big_room;
	element.next_room = big_room_component.first_room;
	big_room_component.first_room = room;
}
