#pragma once

#include <memory>
#include <random>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "tiny_ecs_registry.hpp"

class CombatSystem {
public:
	bool do_attack(Stats& attacker, Attack& attack, Stats& target);
	void init(std::shared_ptr<std::default_random_engine> rng);

private:
	std::shared_ptr<std::default_random_engine> rng;
};