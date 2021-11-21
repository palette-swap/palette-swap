#include "combat_system.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/schema.h>

void CombatSystem::init(std::shared_ptr<std::default_random_engine> global_rng,
						std::shared_ptr<AnimationSystem> animation_system,
						std::shared_ptr<MapGeneratorSystem> map_generator_system)
{
	this->rng = std::move(global_rng);
	this->animations = std::move(animation_system);
	this->map = std::move(map_generator_system);

	load_items();
}

void CombatSystem::restart_game()
{
	looted = 0;
	loot_misses = 0;
	loot_list.clear();

	std::vector<size_t> used;

	for (auto& list : loot_table) {
		used.push_back(0);
		std::shuffle(list.begin(), list.end(), *rng);
	}

	for (int i = 0; i < loot_count; i++) {
		bool filled = false;
		for (size_t j = 0; j < loot_table.size(); j++) {
			std::uniform_int_distribution<size_t> draw_from_list(0, loot_table.at(j).size() - used.at(j));
			if (draw_from_list(*rng) > 0) {
				loot_list.push_back(loot_table.at(j).at(used.at(j)++));
				filled = true;
				break;
			}
		}
		if (!filled) {
			for (size_t j = 0; j < loot_table.size(); j++) {
				if (used.at(j) < loot_table.at(j).size()) {
					loot_list.push_back(loot_table.at(j).at(used.at(j)++));
					break;
				}
			}
		}
	}
}

bool CombatSystem::try_pickup_items(Entity player)
{
	MapPosition& player_pos = registry.get<MapPosition>(player);
	for (auto [entity, pos] : registry.view<HealthPotion, MapPosition>().each()) {
		if (player_pos.position == pos.position) {
			registry.get<Inventory>(player).health_potions++;
			registry.destroy(entity);

			for (const auto& callback : pickup_callbacks) {
				callback(entity, MAXSIZE_T);
			}
			return true;
		}
	}
	for (auto [entity, item, pos] : registry.view<Item, MapPosition>().each()) {
		if (player_pos.position == pos.position) {
			Inventory& inventory = registry.get<Inventory>(player);
			for (size_t i = 0; i < Inventory::inventory_size; i++) {
				if (inventory.inventory.at(i) == entt::null) {
					inventory.inventory.at(i) = item.item_template;
					registry.destroy(entity);

					for (const auto& callback : pickup_callbacks) {
						callback(inventory.inventory.at(i), i);
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool CombatSystem::try_drink_potion(Entity player)
{
	Inventory& inventory = registry.get<Inventory>(player);
	if (inventory.health_potions == 0) {
		return false;
	}
	inventory.health_potions--;
	Stats& stats = registry.get<Stats>(player);
	stats.health = stats.health_max;
	return true;
}

bool CombatSystem::is_valid_attack(Entity attacker, Attack& attack, uvec2 target)
{
	if (!can_reach(attacker, attack, target) || registry.get<Stats>(attacker).mana < attack.mana_cost) {
		return false;
	}
	uvec2 attacker_pos = registry.get<MapPosition>(attacker).position;
	for (const auto& target_entity : registry.view<Enemy, Stats>()) {
		if (attack.is_in_range(attacker_pos, target, registry.get<MapPosition>(target_entity).position)) {

			return true;
		}
	}
	return false;
}

bool CombatSystem::do_attack(Entity attacker, Attack& attack, uvec2 target)
{
	if (!can_reach(attacker, attack, target) || registry.get<Stats>(attacker).mana < attack.mana_cost) {
		return false;
	}
	registry.get<Stats>(attacker).mana -= attack.mana_cost;
	bool success = false;
	for (const auto& target_entity : registry.view<Enemy, Stats>()) {
		if (registry.get<MapPosition>(target_entity).position == target) {
			success |= do_attack(attacker, attack, target_entity);
		}
	}
	return success;
}

bool CombatSystem::do_attack(Entity attacker_entity, Attack& attack, Entity target_entity)
{
	// Checks that the attacker and the enemy wasn't the same entity
	if (attacker_entity == target_entity) {
		return false;
	}

	if (registry.any_of<AOESquare>(attacker_entity)) {
		// Attacker is an AOE square.
		
		// TODO (Evan): explode the AOE square and make it invisible after the explosion animation.
		// (Don't worry about destroying AOE square entity since the AI system will do so in the next turn.)
		// 
		// First set a stable state which makes AOE square invisible.
		// Then add a temporary explosion animation event to the AOE square.

	} else {
		MapPosition& attacker_position = registry.get<MapPosition>(attacker_entity);
		MapPosition& target_position = registry.get<MapPosition>(target_entity);
		// Changes attacker's direction based on the location of the target relative to
		// the target entity
		if (target_position.position.x < attacker_position.position.x) {
			animations->set_sprite_direction(attacker_entity, Sprite_Direction::SPRITE_LEFT);
		} else {
			animations->set_sprite_direction(attacker_entity, Sprite_Direction::SPRITE_RIGHT);
		}

		// Triggers attack based on player/enemy attack prescence
		animations->attack_animation(attacker_entity);
	}

	animations->damage_animation(target_entity);

	Stats& attacker = registry.get<Stats>(attacker_entity);
	Stats& target = registry.get<Stats>(target_entity);
	// Roll a random to hit between min and max (inclusive), add attacker's to_hit_bonus
	std::uniform_int_distribution<int> attack_roller(attack.to_hit_min, attack.to_hit_max);
	int attack_roll = attack_roller(*rng) + attacker.to_hit_bonus;

	// It hits if the attack roll is >= the target's evasion
	bool success = attack_roll >= target.evasion;
	if (success) {
		// Roll a random damage between min and max (inclusive), then
		// add attacker's damage_bonus and the target's damage_modifiers
		std::uniform_int_distribution<int> damage_roller(attack.damage_min, attack.damage_max);
		target.health -= max(damage_roller(*rng) + attacker.damage_bonus
								 + target.damage_modifiers[static_cast<int>(attack.damage_type)],
							 0);
		do_attack_effects(attacker_entity, attack, target_entity);
	}

	for (const auto& callback : attack_callbacks) {
		callback(attacker_entity, target_entity);
	}

	if (target.health <= 0 && !registry.any_of<Player>(target_entity)) {
		kill(attacker_entity, target_entity);
	}

	return success;
}

void CombatSystem::drop_loot(uvec2 position)
{
	// Initial Drop rates are as follows
	// 1-3: Nothing
	// 4-5: Health Potion
	// 6-9: Item Drop
	// If all items have dropped, only drop health potions as follows:
	// 1-6: Nothing
	// 7-9: Health Potion

	// This is tempered by increasing the floor by the number of consecutive misses
	bool all_dropped = looted >= loot_list.size();
	std::uniform_int_distribution<size_t> drop_chance(1 + loot_misses, 9);
	size_t result = drop_chance(*rng);
	if (result <= 3 || (all_dropped && result <= 6)) {
		loot_misses++;
		return;
	}
	loot_misses = 0;
	if (result <= 5 || all_dropped) {
		// Health Potion
		Entity potion = registry.create();
		registry.emplace<HealthPotion>(potion);
		registry.emplace<MapPosition>(potion, position);
		registry.emplace<RenderRequest>(
			potion, TEXTURE_ASSET_ID::CANNONBALL, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE, true);
		registry.emplace<Color>(potion, vec3(1, .1, .1));
		return;
	}
	Entity template_entity = loot_list.at(looted++ % loot_list.size());
	Entity loot = registry.create();
	registry.emplace<Item>(loot, template_entity);
	registry.emplace<MapPosition>(loot, position);
	registry.emplace<RenderRequest>(
		loot, TEXTURE_ASSET_ID::CANNONBALL, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE, true);
	registry.emplace<Color>(loot, vec3(.8314f, .6863f, .2157f) * 1.1f);
}

void CombatSystem::kill(Entity attacker_entity, Entity target_entity)
{
	Stats& stats = registry.get<Stats>(attacker_entity);
	// Regen 25% of total mana with a successful kill
	stats.mana = min(stats.mana_max, stats.mana + stats.mana_max / 4);

	drop_loot(registry.get<MapPosition>(target_entity).position);

	// TODO: Animate death
	registry.destroy(target_entity);

	for (const auto& callback : death_callbacks) {
		callback(target_entity);
	}
}

void CombatSystem::on_attack(
	const std::function<void(const Entity& attacker, const Entity& target)>& do_attack_callback)
{
	attack_callbacks.push_back(do_attack_callback);
}

void CombatSystem::on_pickup(const std::function<void(const Entity& item, size_t slot)>& on_pickup_callback)
{
	pickup_callbacks.push_back(on_pickup_callback);
}

void CombatSystem::on_death(const std::function<void(const Entity& entity)>& on_death_callback)
{
	death_callbacks.push_back(on_death_callback);
}

std::string CombatSystem::make_attack_list(const Entity entity, size_t current_attack) const
{
	Weapon& weapon = registry.get<Weapon>(entity);
	std::ostringstream attacks;
	for (size_t i = 0; i < weapon.given_attacks.size(); i++) {
		const Attack& attack = weapon.get_attack(i);
		if (i == current_attack) {
			attacks << "\n[" << i + 1 << "] ";
		} else {
			attacks << "\n " << i + 1 << "  ";
		}
		attacks << attack.name;
		// Note we could log more details, but it's a bit much visually
		// Maybe on some sort of menu?
		/*attacks << ": " << attack.to_hit_min << "-" << attack.to_hit_max
				<< " to hit, " << attack.damage_min << "-" << attack.damage_max << " "
				<< get_name(attack.damage_type) << " dmg ("
				<< ((attack.targeting_type == TargetingType::Projectile) ? "Projectile)" : "Melee)");*/
	}
	return attacks.str();
}

bool CombatSystem::can_reach(Entity attacker, Attack& attack, uvec2 target)
{
	if (attack.targeting_type == TargetingType::Adjacent) {
		ivec2 distance_vec = abs(ivec2(target - registry.get<MapPosition>(attacker).position));
		int distance = abs(distance_vec.x - distance_vec.y) + min(distance_vec.x, distance_vec.y) * 3 / 2;
		if (distance > attack.range || distance == 0) {
			return false;
		}
	}
	return true;
}

void CombatSystem::do_attack_effects(Entity attacker, Attack& attack, Entity target)
{
	static std::uniform_real_distribution<float> effect_roller(0, 1);
	Entity effect_entity = attack.effects;
	while (effect_entity != entt::null) {
		EffectEntry effect = registry.get<EffectEntry>(effect_entity);
		if (effect_roller(*rng) <= effect.chance) {
			switch (effect.effect) {
			case Effect::Shove: {
				try_shove(attacker, effect, target);
				break;
			}
			case Effect::Stun: {
				Stunned& stunned = registry.get_or_emplace<Stunned>(target, effect.magnitude);
				stunned.rounds = max(stunned.rounds, effect.magnitude);
				break;
			}
			default:
				break;
			}
		}
		effect_entity = effect.next_effect;
	}
}

void CombatSystem::try_shove(Entity attacker, EffectEntry& effect, Entity target)
{
	MapPosition& a_pos = registry.get<MapPosition>(attacker);
	MapPosition& t_pos = registry.get<MapPosition>(target);
	vec2 distance = glm::normalize(vec2(t_pos.position) - vec2(a_pos.position)) * static_cast<float>(effect.magnitude);
	// The amount we need to move
	ivec2 shift = ivec2(roundf(distance.x), roundf(distance.y));
	// The sign of movement in each direction, used for incremental movement
	ivec2 shift_sign = ivec2((shift.x >= 0) ? 1 : -1, (shift.y >= 0) ? 1 : -1);
	while (shift.x != 0 || shift.y != 0) {
		// Each of these helpers moves the player 1 unit along its respective axis if the map allows it
		// and returns whether or not it worked
		auto try_x = [&]() -> bool {
			if (abs(shift.x) > 0 && map->walkable_and_free(uvec2(t_pos.position.x + shift_sign.x, t_pos.position.y))) {
				t_pos.position.x += shift_sign.x;
				shift.x -= shift_sign.x;
				return true;
			}
			return false;
		};
		auto try_y = [&]() -> bool {
			if (abs(shift.y) > 0 && map->walkable_and_free(uvec2(t_pos.position.x, t_pos.position.y + shift_sign.y))) {
				t_pos.position.y += shift_sign.y;
				shift.y -= shift_sign.y;
				return true;
			}
			return false;
		};
		// If we have more y movement left, try to move along y first
		if (abs(shift.x) < abs(shift.y)) {
			if (!try_y() && !try_x()) {
				break;
			}
		}
		// Otherwise, try to move along x first
		else if (!try_x() && !try_y()) {
			break;
		}
	}
}

void CombatSystem::load_items()
{
	std::ifstream file = std::ifstream(json_schema_path("items_schema.json"));
	rapidjson::IStreamWrapper schema_wrapper(file);
	rapidjson::Document schema_doc;
	schema_doc.ParseStream(schema_wrapper);
	rapidjson::SchemaDocument schema(schema_doc);
	for (const auto& entry : std::filesystem::recursive_directory_iterator(data_path() + "/items/")) {
		file = std::ifstream(entry.path());
		rapidjson::IStreamWrapper wrapper(file);
		rapidjson::Document json_doc;
		rapidjson::SchemaValidatingReader<rapidjson::kParseDefaultFlags, rapidjson::IStreamWrapper, rapidjson::UTF8<>>
			reader(wrapper, schema);
		json_doc.Populate(reader);

		assert(reader.GetParseResult());

		assert(json_doc.IsArray());

		for (rapidjson::SizeType i = 0; i < json_doc.Size(); i++) {
			Entity item_entity = registry.create();
			ItemTemplate& item_component = registry.emplace<ItemTemplate>(item_entity, "");
			item_component.deserialize(item_entity, json_doc[i].GetObj());
			while (loot_table.size() <= item_component.tier) {
				loot_table.emplace_back();
			}
			loot_table.at(item_component.tier).push_back(item_entity);
		}

		loot_count += json_doc.Size();
	}
}
