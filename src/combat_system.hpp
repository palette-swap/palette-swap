#pragma once

#include <memory>
#include <random>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "tiny_ecs_registry.hpp"

class CombatSystem {
public:
	void init(std::shared_ptr<std::default_random_engine> rng);

	bool do_attack(Stats& attacker, Attack& attack, Stats& target);

	// Observer Pattern: attach a callback of do_attack().
	void attach_do_attack_callback(
		const std::function<void(const Entity& attacker, const Entity& target)>& do_attack_callback);

private:
	// Observer Pattern: callbacks of do_attack().
	std::vector<std::function<void(const Entity& attacker, const Entity& target)>> do_attack_callbacks;

	std::shared_ptr<std::default_random_engine> rng;
};
