#pragma once
#include "components.hpp"

#include "tutorial_system.hpp"

#include <random>

class LootSystem {
public:
	void init(std::shared_ptr<std::default_random_engine> global_rng, std::shared_ptr<TutorialSystem> tutorial_system);

	void restart_game();

	size_t get_max_tier() const { return loot_table.size() - 1; }

	bool try_pickup_items(Entity player);

	void drop_loot(uvec2 center_position, float mode_tier, uint count = 1);
	void drop_item(uvec2 position, float mode_tier);
	void drop_resource_pickup(uvec2 position, Resource resource);

	void on_pickup(const std::function<void(const Entity& item, size_t slot)>& on_pickup_callback);

private:
	// Callbacks of try_pickup_items
	std::vector<std::function<void(const Entity& item, size_t slot)>> pickup_callbacks;

	std::shared_ptr<std::default_random_engine> rng;

	// The number of unique items per level
	size_t loot_count = 0;
	// Lists in ascending tier of all loot, where each level is in a random order
	std::vector<std::vector<Entity>> loot_table;
	// Total number of items looted
	size_t looted = 0;
	// Number of items looted per tier
	std::vector<size_t> looted_per_tier;
	// Number of times consecutively which nothing has been looted
	size_t loot_misses = 0;

	std::shared_ptr<TutorialSystem> tutorials;
};