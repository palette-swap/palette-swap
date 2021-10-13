#pragma once

#include <random>
#include <vector>

#include "common.hpp"
#include "tiny_ecs_registry.hpp"

#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "turn_system.hpp"

class AISystem {
public:
	AISystem(std::shared_ptr<CombatSystem> combat,
			 std::shared_ptr<MapGeneratorSystem> map_generator,
			 std::shared_ptr<TurnSystem> turns);

	void step(float elapsed_ms);

private:

	// Switch enemy state.
	void switch_enemy_state(const Entity& enemy_entity, ENEMY_STATE_ID enemy_state);

	// Check if it is player's turn.
	bool is_player_turn();

	// Switch to player's turn.
	void switch_to_player_turn();

	// Check if the player is spotted by an entity within its radius.
	bool is_player_spotted(const Entity& entity, uint radius);

	// Check if the player is reachable by an entity within its attack range.
	bool is_player_reachable(const Entity& entity, uint attack_range);

	// Check if an entity is afraid.
	bool is_afraid(const Entity& entity);

	// Check if an entity is at its nest.
	bool is_at_nest(const Entity& entity);

	// An entity becomes alert.
	void become_alert(const Entity& entity);

	// An entity attackes the player.
	void attack_player(const Entity& entity);

	// An entity approaches the player.
	bool approach_player(const Entity& entity);

	// An entity approaches its nest.
	bool approach_nest(const Entity& entity);

	// An entity moves to a targeted map position.
	bool move(const Entity& entity, const uvec2& map_pos);

	std::shared_ptr<CombatSystem> combat;
	std::shared_ptr<MapGeneratorSystem> map_generator;
	std::shared_ptr<TurnSystem> turns;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1

	// Entity representing the enemy team's turn
	Entity enemy_team;
};
