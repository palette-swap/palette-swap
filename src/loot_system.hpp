#pragma once
#include "components.hpp"

#include "tutorial_system.hpp"

#include <random>

class LootSystem {
public:
	void init(std::shared_ptr<std::default_random_engine> global_rng, std::shared_ptr<TutorialSystem> tutorial_system);

	void restart_game();

	bool try_pickup_items(Entity player);

	void drop_loot(uvec2 center_position);
	void drop_item(uvec2 position);
	void drop_resource_pickup(uvec2 position, Resource resource);

	void on_pickup(const std::function<void(const Entity& item, size_t slot)>& on_pickup_callback);

private:
	// Callbacks of try_pickup_items
	std::vector<std::function<void(const Entity& item, size_t slot)>> pickup_callbacks;

	std::shared_ptr<std::default_random_engine> rng;

	size_t loot_count = 0;
	std::vector<std::vector<Entity>> loot_table;
	size_t looted = 0;
	size_t loot_misses = 0;
	std::vector<Entity> loot_list;

	std::shared_ptr<TutorialSystem> tutorials;
};