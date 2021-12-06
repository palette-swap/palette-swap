#pragma once

#include "common.hpp"
#include "components.hpp"
#include "map_generator_system.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem {
public:
	explicit PhysicsSystem(const Debug& debugging, std::shared_ptr<MapGeneratorSystem> map);

	void step(float elapsed_ms, float /*window_width*/, float /*window_height*/);

private:
	template <typename ColorExclusive>
	void check_occupied(const std::vector<uvec2>& tiles, Entity entity, Entity shooter);

	const Debug& debugging;
	const std::shared_ptr<MapGeneratorSystem> map_generator;
};

template <typename ColorExclusive>
void PhysicsSystem::check_occupied(const std::vector<uvec2>& tiles, Entity entity, Entity shooter)
{
	for (auto [entity_j, map_position_j] :
		 registry.view<MapPosition>(entt::exclude<MapHitbox, Item, ColorExclusive, ResourcePickup, Environmental>)
			 .each()) {
		if (entity_j == shooter) {
			continue;
		}
		if (std::find(tiles.begin(), tiles.end(), map_position_j.position) != tiles.end()) {
			Collision::add(entity, entity_j);
		}
	}
	for (auto [entity_j, map_position_j, hitbox_j] :
		 registry.view<MapPosition, MapHitbox>(entt::exclude<Item, ColorExclusive, ResourcePickup, Environmental>)
			 .each()) {
		if (entity_j == shooter) {
			continue;
		}
		for (auto square : MapUtility::MapArea(map_position_j, hitbox_j)) {
			if (std::find(tiles.begin(), tiles.end(), square) != tiles.end()) {
				Collision::add(entity, entity_j);
				break;
			}
		}
	}
}
