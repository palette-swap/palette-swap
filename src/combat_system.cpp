#include "combat_system.hpp"

#include <sstream>

void CombatSystem::init(std::shared_ptr<std::default_random_engine> rng) { this->rng = std::move(rng); }

bool CombatSystem::do_attack(Entity attacker_entity, Attack& attack, Entity target_entity)
{
	Stats& attacker = registry.stats.get(attacker_entity);
	Stats& target = registry.stats.get(target_entity);
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
		printf("Hit! Target's new HP is %i\n", target.health);
	} else {
		printf("Miss!\n");
	}

	// TODO: replace Entity a and t with args of do_attack() after Dylan's restructure of combat system.
	// This currently has no effects.
	for (const auto& callback : do_attack_callbacks) {
		callback(attacker_entity, target_entity);
	}

	return success;
}

void CombatSystem::attach_do_attack_callback(
	const std::function<void(const Entity& attacker, const Entity& target)>& do_attack_callback)
{
	do_attack_callbacks.push_back(do_attack_callback);
}

std::string get_name(DamageType d)
{
	switch (d) {
	case DamageType::Magical:
		return "magic";
	case DamageType::Fire:
		return "fire";
	default:
	case DamageType::Physical:
		return "physical";
	}
}

std::string CombatSystem::make_attack_list(const Entity entity, size_t current_attack) const {
	const Weapon& weapon = registry.weapons.get(entity);
	std::ostringstream attacks;
	for (size_t i = 0; i < weapon.given_attacks.size(); i++) {
		const Attack& attack = weapon.given_attacks[i];
		if (i == current_attack) {
			attacks << "\n[" << i + 1 << "] ";
		} else {
			attacks << "\n " << i + 1 << "  ";
		}
		attacks << attack.name << ": " << attack.to_hit_min << "-" << attack.to_hit_max
				<< " to hit, " << attack.damage_min << "-" << attack.damage_max << " "
				<< get_name(attack.damage_type) << " dmg ("
				<< ((attack.targeting_type == TargetingType::Projectile) ? "Projectile)" : "Melee)");
	}
	return attacks.str();
}
