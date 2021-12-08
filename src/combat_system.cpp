#include "combat_system.hpp"

#include <sstream>

void CombatSystem::init(std::shared_ptr<std::default_random_engine> global_rng,
						std::shared_ptr<AnimationSystem> animation_system,
						std::shared_ptr<LootSystem> loot_system,
						std::shared_ptr<MapGeneratorSystem> map_generator_system,
						std::shared_ptr<TutorialSystem> tutorial_system)
{
	this->rng = std::move(global_rng);
	this->animations = std::move(animation_system);
	this->loot = std::move(loot_system);
	this->map = std::move(map_generator_system);
	this->tutorials = std::move(tutorial_system);
}

bool CombatSystem::try_drink_potion(Entity player)
{
	Inventory& inventory = registry.get<Inventory>(player);
	if (inventory.resources.at((size_t)Resource::HealthPotion) == 0) {
		return false;
	}
	inventory.resources.at((size_t)Resource::HealthPotion)--;
	Stats& stats = registry.get<Stats>(player);
	stats.health = stats.health_max;
	return true;
}

int CombatSystem::get_effect(Entity entity, Effect effect) const
{
	ActiveConditions* conditions = registry.try_get<ActiveConditions>(entity);
	if (conditions == nullptr) {
		return 0;
	}
	return conditions->conditions.at((size_t)effect);
}

int CombatSystem::get_decrement_effect(Entity entity, Effect effect)
{
	ActiveConditions* conditions = registry.try_get<ActiveConditions>(entity);
	if (conditions == nullptr || conditions->conditions.at((size_t)effect) == 0) {
		return 0;
	}
	return conditions->conditions.at((size_t)effect)--;
}

void CombatSystem::apply_decrement_per_turn_effects(Entity entity)
{
	for (int i = 0; i < num_per_turn_conditions; i++) {
		auto effect = (Effect)(num_per_use_conditions + i);
		int amount = get_decrement_effect(entity, effect);
		if (amount <= 0) {
			continue;
		}
		switch (effect) {
		case Effect::Bleed:
		case Effect::Burn: {
			DamageType type = (effect == Effect::Bleed) ? DamageType::Physical : DamageType::Fire;
			Stats& stats = registry.get<Stats>(entity);
			stats.health -= max(amount + stats.damage_modifiers[static_cast<int>(type)], 0);
			break;
		}
		default:
			break;
		}
	}
}

bool CombatSystem::is_valid_attack(Entity attacker, Attack& attack, uvec2 target)
{
	ColorState& inactive_color = registry.get<PlayerInactivePerception>(registry.view<Player>().front()).inactive;
	if (inactive_color == ColorState::Red) {
		return is_valid_attack<RedExclusive>(attacker, attack, target);
	}
	return is_valid_attack<BlueExclusive>(attacker, attack, target);
}

template <typename ColorExclusive> bool CombatSystem::is_valid_attack(Entity attacker, Attack& attack, uvec2 target)
{
	if (!attack.can_reach(attacker, target) || registry.get<Stats>(attacker).mana < attack.mana_cost) {
		return false;
	}
	uvec2 attacker_pos = registry.get<MapPosition>(attacker).position;
	auto view = registry.view<MapPosition, Enemy, Stats>(entt::exclude<ColorExclusive, Environmental>);
	for (const Entity target_entity : view) {
		if (target_entity != attacker
			&& attack.is_in_range(attacker_pos, target, view.template get<MapPosition>(target_entity).position)) {

			return true;
		}
	}
	auto view_big = registry.view<MapPosition, MapHitbox, Enemy, Stats>(entt::exclude<ColorExclusive, Environmental>);
	for (const Entity target_entity : view_big) {
		for (auto square : MapUtility::MapArea(view_big.template get<MapPosition>(target_entity),
											   view_big.template get<MapHitbox>(target_entity))) {
			if (target_entity != attacker && attack.is_in_range(attacker_pos, target, square)) {
				return true;
			}
		}
	}
	return false;
}

bool CombatSystem::do_attack(Entity attacker, Attack& attack, uvec2 target)
{
	ColorState& inactive_color = registry.get<PlayerInactivePerception>(registry.view<Player>().front()).inactive;
	if (inactive_color == ColorState::Red) {
		return do_attack<RedExclusive>(attacker, attack, target);
	}
	return do_attack<BlueExclusive>(attacker, attack, target);
}

template <typename ColorExclusive> bool CombatSystem::do_attack(Entity attacker, Attack& attack, uvec2 target)
{
	if (!attack.can_reach(attacker, target) || registry.get<Stats>(attacker).mana < attack.mana_cost) {
		return false;
	}
	if (attack.turn_cost > 1) {
		registry.get_or_emplace<ActiveConditions>(attacker).conditions.at((size_t)Effect::Stun) = attack.turn_cost - 1;
	}
	registry.get<Stats>(attacker).mana -= attack.mana_cost;
	bool success = false;
	uvec2 attacker_pos = registry.get<MapPosition>(attacker).position;
	auto try_attack = [&](Entity target_entity, uvec2 pos) -> bool {
		if (attack.is_in_range(attacker_pos, target, pos)) {
			if (attack.mana_cost != 0) {
				animations->player_spell_impact_animation(target_entity, attack.damage_type);
			}
			success |= do_attack(attacker, attack, target_entity);
			return true;
		}
		return false;
	};
	auto view = registry.view<MapPosition, Enemy, Stats>(entt::exclude<MapHitbox, ColorExclusive, Environmental>);
	for (const Entity target_entity : view) {
		try_attack(target_entity, view.template get<MapPosition>(target_entity).position);
	}
	auto view_big = registry.view<MapPosition, MapHitbox, Enemy, Stats>(entt::exclude<ColorExclusive, Environmental>);
	for (const Entity target_entity : view_big) {
		for (auto square : MapUtility::MapArea(view_big.template get<MapPosition>(target_entity),
											   view_big.template get<MapHitbox>(target_entity))) {
			if (try_attack(target_entity, square)) {
				break;
			}
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
	// Roll a random to hit between min and max (inclusive), add attacker's to_hit_bonus and subtract disarmed amount
	std::uniform_int_distribution<int> attack_roller(attack.to_hit_min, attack.to_hit_max);
	int hit_bonus = (attack.mana_cost != 0) ? attacker.to_hit_spells : attacker.to_hit_weapons;
	int attack_roll = attack_roller(*rng) + hit_bonus - get_effect(attacker_entity, Effect::Disarm);

	// It hits if the attack roll is >= the target's evasion minus entangled amount
	bool success = attack_roll >= target.evasion - get_effect(target_entity, Effect::Entangle);
	if (success) {
		// Roll a random damage between min and max (inclusive), then
		// add attacker's damage_bonus and the target's damage_modifiers, take away weakened amount
		std::uniform_int_distribution<int> damage_roller(attack.damage_min, attack.damage_max);
		size_t type = static_cast<size_t>(attack.damage_type);
		int dmg = max(damage_roller(*rng) + attacker.damage_bonus.at(type) - get_effect(attacker_entity, Effect::Weaken)
						  + target.damage_modifiers[type],
					  0);
		target.health -= dmg;
		do_attack_effects(attacker_entity, attack, target_entity, dmg);
	}

	for (const auto& callback : attack_callbacks) {
		callback(attacker_entity, target_entity);
	}

	if (target.health <= 0 && !registry.any_of<Player>(target_entity)) {
		kill(attacker_entity, target_entity);
	}

	return success;
}

void CombatSystem::kill(Entity attacker_entity, Entity target_entity)
{
	const Enemy& enemy = registry.get<Enemy>(target_entity);
	Stats& stats = registry.get<Stats>(attacker_entity);

	if (enemy.type == EnemyType::TrainingDummy) {
		// Regen 100% of total mana with a successful kill of training dummies.
		stats.mana = stats.mana_max;
	} else {
		// Regen 25% of total mana with a successful kill.
		stats.mana = min(stats.mana_max, stats.mana + stats.mana_max / 4);
	}

	if (Inventory* inventory = registry.try_get<Inventory>(attacker_entity)) {
		if (inventory->resources.at((size_t)Resource::PaletteSwap) < 5) {
			inventory->resources.at((size_t)Resource::PaletteSwap)++;
		}
	}

	float mode_tier = static_cast<float>(enemy.danger_rating) / static_cast<float>(max_danger_rating)
		* static_cast<float>(loot->get_max_tier() - 1) + .5f;
	loot->drop_loot(registry.get<MapPosition>(target_entity).position, mode_tier, enemy.loot_multiplier);

	// TODO: Animate death
	animations->set_enemy_death_animation(target_entity);

	for (const auto& callback : death_callbacks) {
		callback(target_entity);
	}

	// TODO: Animate death
	registry.destroy(target_entity);
}

void CombatSystem::on_attack(
	const std::function<void(const Entity& attacker, const Entity& target)>& do_attack_callback)
{
	attack_callbacks.push_back(do_attack_callback);
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

void CombatSystem::do_attack_effects(Entity attacker, Attack& attack, Entity target, int damage)
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
			case Effect::Crit: {
				registry.get<Stats>(target).health -= damage * (effect.magnitude - 1);
				break;
			}
			default: {
				auto effect_index = (size_t)effect.effect;
				assert(effect_index < num_conditions);
				ActiveConditions& conditions = registry.get_or_emplace<ActiveConditions>(target);
				conditions.conditions.at(effect_index) = max(effect.magnitude, conditions.conditions.at(effect_index));
				break;
			}
			}
		}
		effect_entity = effect.next_effect;
	}
}

void CombatSystem::try_shove(Entity attacker, EffectEntry& effect, Entity target)
{
	if (registry.any_of<MapHitbox>(target)) {
		// Can't shove big creatures
		return;
	}
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
			if (abs(shift.x) > 0
				&& map->walkable_and_free(target, uvec2(t_pos.position.x + shift_sign.x, t_pos.position.y))) {
				t_pos.position.x += shift_sign.x;
				shift.x -= shift_sign.x;
				return true;
			}
			return false;
		};
		auto try_y = [&]() -> bool {
			if (abs(shift.y) > 0
				&& map->walkable_and_free(target, uvec2(t_pos.position.x, t_pos.position.y + shift_sign.y))) {
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
