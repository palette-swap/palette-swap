#pragma once

#include <memory>
#include <random>

#include "animation_system.hpp"
#include "loot_system.hpp"
#include "map_generator_system.hpp"
#include "tutorial_system.hpp"

#include "common.hpp"
#include "components.hpp"

class CombatSystem {
public:

	void init(std::shared_ptr<std::default_random_engine> global_rng,
			  std::shared_ptr<AnimationSystem> animation_system,
			  std::shared_ptr<LootSystem> loot_system,
			  std::shared_ptr<MapGeneratorSystem> map_generator_system,
			  std::shared_ptr<TutorialSystem> tutorial_system);

	bool try_drink_potion(Entity player);

	// Conditions
	int get_effect(Entity entity, Effect effect) const;
	int get_decrement_effect(Entity entity, Effect effect);
	void apply_decrement_per_turn_effects(Entity entity);

	bool is_valid_attack(Entity attacker, Attack& attack, uvec2 target);
	template <typename ColorExclusive> bool is_valid_attack(Entity attacker, Attack& attack, uvec2 target);
	bool do_attack(Entity attacker, Attack& attack, uvec2 target);
	template <typename ColorExclusive> bool do_attack(Entity attacker, Attack& attack, uvec2 target);
	bool do_attack(Entity attacker, Attack& attack, Entity target);

	// Observer Pattern: attach a callback of do_attack().
	void on_attack(const std::function<void(const Entity& attacker, const Entity& target)>& do_attack_callback);
	void on_death(const std::function<void(const Entity& entity)>& on_death_callback);

	std::string make_attack_list(Entity entity, size_t current_attack) const;

private:

	// Private attack helpers
	bool can_reach(Entity attacker, Attack& attack, uvec2 target);
	void kill(Entity attacker_entity, Entity entity);

	// Attack Effects e.g. Shove, Stun
	void do_attack_effects(Entity attacker, Attack& attack, Entity target, int damage);
	void try_shove(Entity attacker, EffectEntry& effect, Entity target);

	// Observer Pattern: callbacks of do_attack().
	std::vector<std::function<void(const Entity& attacker, const Entity& target)>> attack_callbacks;
	// Callbacks when an entity dies
	std::vector<std::function<void(const Entity& entity)>> death_callbacks;

	std::shared_ptr<std::default_random_engine> rng;

	std::shared_ptr<AnimationSystem> animations;
	std::shared_ptr<LootSystem> loot;
	std::shared_ptr<MapGeneratorSystem> map;
	std::shared_ptr<TutorialSystem> tutorials;
};
