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
	for (auto [entity, velocity, position]: registry.view<Velocity, WorldPosition>().each()) {
		float step_seconds = 1.0f * (elapsed_ms / 1000.f);
		position.position += velocity.get_velocity() * step_seconds;
	}

	// Check for collisions between projectiles and living entities(player and enemy)
	// Note the physic system modules should only be used for projectiles,
	// player and enemy collisions don't need to be considered because
	// their movements are tile-based, projectiles are the only exception.
	for (auto [entity_i, projectile, world_pos] : registry.view<ActiveProjectile, WorldPosition>().each()) {

		if (debugging.in_debug_mode) {
			create_line(world_pos.position, projectile.radius * 2.f, 0);
			create_line(world_pos.position, projectile.radius * 2.f, M_PI / 2.f);
		}

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
		for (auto [entity_j, map_position_j] : registry.view<MapPosition>().each()) {
			if (entity_j == projectile.shooter) {
				continue;
			}
			if (std::find(tiles.begin(), tiles.end(), map_position_j.position) != tiles.end()) {
				Collision* collision = registry.try_get<Collision>(entity_i);
				if (collision == nullptr) {
					collision = &(registry.emplace<Collision>(entity_i, registry.create()));
					registry.emplace<Child>(collision->children, entity_i, entt::null, entity_j);
				} else {
					Entity new_collision = registry.create();
					registry.emplace<Child>(new_collision, entity_i, collision->children, entity_j);
					collision->children = new_collision;
				}
				return;
			}
		}
		for (uvec2 tile : tiles) {
			// Check if projectile hits a wall
			if (map_generator->is_wall(tile) || !map_generator->is_on_map(tile)) {
				if (registry.try_get<Collision>(entity_i) == nullptr) {
					registry.emplace<Collision>(entity_i, entt::null);
				}
				return;
			}
		}
	}
}
