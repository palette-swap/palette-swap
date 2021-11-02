// internal
#include "physics_system.hpp"
#include "world_init.hpp"

#include "geometry.hpp"

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box()
{
	// abs is to avoid negative scale due to the facing direction.
	// TODO: This needs to be reworked, as we move scale to be statically defined
	// array, this method can potentially take in a TEXTURE_ASSET_ID, depending on
	// how exactly we want to do collision
	return { abs(MapUtility::tile_size), abs(MapUtility::tile_size) };
}

PhysicsSystem::PhysicsSystem(const Debug& debugging, std::shared_ptr<MapGeneratorSystem> map)
	: debugging(debugging)
	, map_generator(std::move(map))
{
}

void PhysicsSystem::step(float elapsed_ms, float /*window_width*/, float /*window_height*/)
{
	// Currently still using motion component to udpate projectile position based on velocity
	// TODO: Change check for motions into check for projectiles, update based on projectile component
	auto& velocity_registry = registry.velocities;
	for (uint i = 0; i < velocity_registry.size(); i++) {
		Velocity& velocity = velocity_registry.components[i];
		Entity entity = velocity_registry.entities[i];
		WorldPosition& position = registry.world_positions.get(entity);
		float step_seconds = 1.0f * (elapsed_ms / 1000.f);
		position.position += velocity.get_velocity() * step_seconds;
	}

	// Check for collisions between projectiles and living entities(player and enemy)
	// Note the physic system modules should only be used for projectiles,
	// player and enemy collisions don't need to be considered because
	// their movements are tile-based, projectiles are the only exception.
	ComponentContainer<WorldPosition>& world_positions = registry.world_positions;
	ComponentContainer<MapPosition>& map_positions = registry.map_positions;
	for (uint i = 0; i < world_positions.components.size(); i++) {
		Entity entity_i = world_positions.entities[i];
		if (!registry.active_projectiles.has(entity_i)) {
			continue;
		}
		WorldPosition& world_pos = world_positions.components[i];
		ActiveProjectile& projectile = registry.active_projectiles.get(entity_i);
		Geometry::Circle collider = { world_pos.position, projectile.radius };
		std::vector<uvec2> tiles
			= MapUtility::get_surrounding_tiles(MapUtility::world_position_to_map_position(collider.center),
												floor(1 + projectile.radius * 2.f / MapUtility::tile_size));

		tiles.erase(std::remove_if(tiles.begin(),
								   tiles.end(),
								   [&collider](const uvec2& tile) {
									   vec2 center = MapUtility::map_position_to_world_position(tile);
									   vec2 size = get_bounding_box();
									   return !Geometry::Rectangle(center, size).intersects(collider);
								   }),
					tiles.end());

		// Check if the map position is occupy
		for (uint j = 0; j < map_positions.components.size(); j++) {
			Entity entity_j = map_positions.entities[j];
			if (entity_j == projectile.shooter) {
				continue;
			}
			MapPosition& map_position_j = map_positions.components[j];
			if (std::find(tiles.begin(), tiles.end(), map_position_j.position) != tiles.end()) {
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
				return;
			}
		}
		for (uvec2 tile : tiles) {
			// Check if projectile hits a wall
			if (map_generator->is_wall(tile) || !map_generator->is_on_map(tile)) {
				registry.collisions.emplace_with_duplicates(entity_i, Entity::undefined());
				return;
			}
		}
	}
}
