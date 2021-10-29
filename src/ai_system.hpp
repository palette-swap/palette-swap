#pragma once

#include <random>
#include <vector>

#include "common.hpp"
#include "tiny_ecs_registry.hpp"

#include "combat_system.hpp"
#include "map_generator_system.hpp"
#include "turn_system.hpp"

#include "soloud_wav.h"

class AISystem {
public:
	AISystem(std::shared_ptr<CombatSystem> combat,
			 std::shared_ptr<MapGeneratorSystem> map_generator,
			 std::shared_ptr<TurnSystem> turns);

	void step(float elapsed_ms);

private:
	// Execute state machine of Slime.
	void execute_Slime(const Entity& slime);

	// Execute state machine of Raven.
	void execute_Raven(const Entity& raven);

	// Execute state machine of Living Armor.
	void execute_LivingArmor(const Entity& living_armor);

	// Execute state machine of Tree Ant.
	void execute_TreeAnt(const Entity& tree_ant);

	// Remove an entity if it is dead.
	bool remove_dead_entity(const Entity& entity);

	// Switch enemy state.
	void switch_enemy_state(const Entity& enemy_entity, EnemyState new_state);

	// Check if the player is spotted by an entity within its radius.
	bool is_player_spotted(const Entity& entity, uint radius);

	// Check if the player is reachable by an entity within its attack range.
	bool is_player_in_attack_range(const Entity& entity, uint attack_range);

	// Check if an entity is at its nest.
	bool is_at_nest(const Entity& entity);

	// An entity attackes the player.
	void attack_player(const Entity& entity);

	// An entity approaches the player.
	bool approach_player(const Entity& entity, uint speed);

	// An entity approaches its nest.
	bool approach_nest(const Entity& entity, uint speed);

	// An entity moves to a targeted map position.
	bool move(const Entity& entity, const uvec2& map_pos);

	void recover_health(const Entity& entity, float ratio);

	// Check if an entity's health is below a ratio.
	bool is_health_below(const Entity& entity, float ratio);

	// An entity become immortal if flag is true. Otherwise cancel immortality.
	void become_immortal(const Entity& entity, bool flag);

	// An entity become powerup if flag is true. Otherwise cancel powerup.
	void become_powerup(const Entity& entity, bool flag);

	// Shared resource: Combat system.
	std::shared_ptr<CombatSystem> combat;

	// Shared resource: Map system.
	std::shared_ptr<MapGeneratorSystem> map_generator;

	// Shared resource: Turn system.
	std::shared_ptr<TurnSystem> turns;

	// Entity representing the enemy team's turn.
	Entity enemy_team;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1

	SoLoud::Wav enemy_attack1_wav;
};
