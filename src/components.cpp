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

void MapPosition::deserialize(const std::string& prefix, const rapidjson::Document& json)
{
	const auto* position_x = get_and_assert_value_from_json(prefix + "/position/x", json);
	position.x = position_x->GetInt();
	const auto* position_y = get_and_assert_value_from_json(prefix + "/position/y", json);
	position.y = position_y->GetInt();
}

void Enemy::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/team").c_str()), static_cast<int>(team));
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/type").c_str()), static_cast<int>(type));
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/state").c_str()), static_cast<int>(state));
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/radius").c_str()), radius);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/speed").c_str()), speed);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/attack_range").c_str()), attack_range);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/nest_position/x").c_str()), nest_map_pos.x);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/nest_position/y").c_str()), nest_map_pos.y);
}

void Enemy::deserialize(const std::string& prefix, const rapidjson::Document& json)
{
	const auto* team_value = get_and_assert_value_from_json(prefix + "/team", json);
	team = static_cast<ColorState>(team_value->GetInt());
	const auto* type_value = get_and_assert_value_from_json(prefix + "/type", json);
	type = static_cast<EnemyType>(type_value->GetInt());
	const auto* state_value = get_and_assert_value_from_json(prefix + "/state", json);
	state = static_cast<EnemyState>(state_value->GetInt());
	const auto* radius_value = get_and_assert_value_from_json(prefix + "/radius", json);
	radius = radius_value->GetInt();
	const auto* speed_value = get_and_assert_value_from_json(prefix + "/speed", json);
	speed = speed_value->GetInt();
	const auto* attack_range_value = get_and_assert_value_from_json(prefix + "/attack_range", json);
	attack_range = attack_range_value->GetInt();
	const auto* nest_pos_x = get_and_assert_value_from_json(prefix + "/nest_position/x", json);
	nest_map_pos.x = nest_pos_x->GetInt();
	const auto* nest_pos_y = get_and_assert_value_from_json(prefix + "/nest_position/y", json);
	nest_map_pos.y = nest_pos_y->GetInt();
}

bool Attack::is_in_range(uvec2 source, uvec2 target, uvec2 pos) const
{
	dvec2 distance = dvec2(target) - dvec2(pos);
	dvec2 attack_direction = dvec2(target) - dvec2(source);
	double attack_angle = atan2(attack_direction.y, attack_direction.x);
	dvec2 aligned_distance = glm::rotate(distance, -attack_angle);
	if (pattern == AttackPattern::Rectangle) {
		return aligned_distance.x <= static_cast<double>(parallel_size)
			&& aligned_distance.y <= static_cast<double>(perpendicular_size);
	}
	if (pattern == AttackPattern::Circle) {
		return pow(aligned_distance.x / parallel_size, 2) + pow(aligned_distance.y / perpendicular_size, 2) <= 1;
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

void Stats::apply(Entity item, bool applying) {
	if (item == entt::null) {
		return;
	}
	StatBoosts* stat_boosts = registry.try_get<StatBoosts>(item);
	if (stat_boosts == nullptr) {
		return;
	}
	int applying_mult = (applying) ? 1 : -1;
	health_max += stat_boosts->health * applying_mult;
	mana_max += stat_boosts->mana * applying_mult;
	to_hit_bonus += stat_boosts->to_hit_bonus * applying_mult;
	damage_bonus += stat_boosts->damage_bonus * applying_mult;
	evasion += stat_boosts->evasion * applying_mult;
	for (size_t i = 0; i < damage_modifiers.size(); i++) {
		damage_modifiers.at(i) += stat_boosts->damage_modifiers.at(i) * applying_mult;
	}
}

void Stats::serialize(const std::string& prefix, rapidjson::Document& json) const
{
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/health").c_str()), health);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/health_max").c_str()), health_max);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/mana").c_str()), mana);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/mana_max").c_str()), mana_max);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/to_hit_bonus").c_str()), to_hit_bonus);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/damage_bonus").c_str()), damage_bonus);
	rapidjson::SetValueByPointer(json, rapidjson::Pointer((prefix + "/evasion").c_str()), evasion);

	base_attack.serialize(prefix + "/attack", json);
	// damage_modifiers don't seem to be needed here?
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
	to_hit_bonus = to_hit_bonus_value->GetInt();
	const auto* damage_bonus_value = get_and_assert_value_from_json(prefix + "/damage_bonus", json);
	damage_bonus = damage_bonus_value->GetInt();
	const auto* evasion_value = get_and_assert_value_from_json(prefix + "/evasion", json);
	evasion = evasion_value->GetInt();
	base_attack.deserialize(prefix + "/attack", json);
}

bool operator==(const Text& t1, const Text& t2) { return t1.text == t2.text && t1.font_size == t2.font_size; }

void Collision::add(Entity parent, Entity child)
{
	Collision* collision = registry.try_get<Collision>(parent);
	if (collision == nullptr) {
		collision = &(registry.emplace<Collision>(parent, registry.create()));
		registry.emplace<Child>(collision->children, parent, entt::null, child);
	} else {
		Entity new_collision = registry.create();
		registry.emplace<Child>(new_collision, parent, collision->children, child);
		collision->children = new_collision;
	}
}

void UIGroup::add_element(Entity group, Entity element, UIElement& ui_element)
{
	if (group == entt::null) {
		return;
	}
	UIGroup& g = registry.get<UIGroup>(group);
	ui_element.next = g.first_element;
	g.first_element = element;
}

void UIGroup::add_text(Entity group, Entity text, UIElement& ui_element)
{
	if (group == entt::null) {
		return;
	}
	UIGroup& g = registry.get<UIGroup>(group);
	ui_element.next = g.first_text;
	g.first_text = text;
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
}

void StatBoosts::deserialize(const rapidjson::GenericObject<false, rapidjson::Value>& boosts) {
	if (boosts.HasMember("health")) {
		health = boosts["health"].GetInt();
	}
	if (boosts.HasMember("mana")) {
		mana = boosts["mana"].GetInt();
	}
	if (boosts.HasMember("to_hit")) {
		to_hit_bonus = boosts["to_hit"].GetInt();
	}
	if (boosts.HasMember("damage")) {
		damage_bonus = boosts["damage"].GetInt();
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
