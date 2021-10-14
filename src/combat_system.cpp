#include "combat_system.hpp"

void CombatSystem::init(std::shared_ptr<std::default_random_engine> rng) { this->rng = std::move(rng); }

bool CombatSystem::do_attack(Stats& attacker, Attack& attack, Stats& target)
{
	// Roll a random to hit between min and max (inclusive), add attacker's to_hit_bonus
	std::uniform_int_distribution<int> attack_roller(attack.to_hit_min, attack.to_hit_max);
	int attack_roll = attack_roller(*rng) + attacker.to_hit_bonus;

	// It hits if the attack roll is >= the target's defence
	bool success = attack_roll >= target.defence;
	if (success) {
		// Roll a random damage between min and max (inclusive), then
		// add attacker's damage_bonus and the target's damage_modifiers
		std::uniform_int_distribution<int> damage_roller(attack.damage_min, attack.damage_max);
		target.health -= max(damage_roller(*rng) + attacker.damage_bonus
			+ target.damage_modifiers[static_cast<int>(attack.damage_type)], 0);
		printf("Hit! Target's new HP is %i\n", target.health);
	} else {
		printf("Miss!\n");
	}

	return success;
}
