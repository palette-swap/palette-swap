#pragma once

#include <random>
#include <vector>

#include "common.hpp"
#include "tiny_ecs_registry.hpp"

#include "map_generator_system.hpp"

class AISystem {
public:

	explicit AISystem(std::shared_ptr<MapGeneratorSystem> map_generator);

	void step(float /*elapsed_ms*/, bool& isPlayerTurn);

private:

	void switch_to_idle(const Entity& enemy_entity);

	void switch_to_active(const Entity& enemy_entity);

	void switch_to_flinched(const Entity& enemy_entity);

	bool is_player_turn();

	void switch_to_player_turn();

	bool is_player_spotted(const Entity& entity, const uint radius);

	bool is_player_reachable(const Entity& entity, const uint attack_range);

	bool is_afraid();

	void become_alert(const Entity& entity);

	void attack_player();

	void approach_player();

	void flee_player();

	std::shared_ptr<MapGeneratorSystem> map_generator;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};
