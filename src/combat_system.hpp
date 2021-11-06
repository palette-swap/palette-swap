#pragma once

#include <memory>
#include <random>

#include "animation_system.hpp"
#include "common.hpp"
#include "components.hpp"

class CombatSystem {
public:
	void init(std::shared_ptr<std::default_random_engine> global_rng, std::shared_ptr<AnimationSystem> animation_system);

	bool do_attack(Entity attacker, Attack& attack, Entity target);

	// Observer Pattern: attach a callback of do_attack().
	void attach_do_attack_callback(
		const std::function<void(const Entity& attacker, const Entity& target)>& do_attack_callback);

	[[nodiscard]] std::string make_attack_list(Entity entity, size_t current_attack) const;

private:
	// Observer Pattern: callbacks of do_attack().
	std::vector<std::function<void(const Entity& attacker, const Entity& target)>> do_attack_callbacks;

	std::shared_ptr<std::default_random_engine> rng;

	std::shared_ptr<AnimationSystem> animations;
};
