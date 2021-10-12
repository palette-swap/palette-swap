// internal
#include "physics_system.hpp"
#include "world_init.hpp"

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box()
{
	// abs is to avoid negative scale due to the facing direction.
	// TODO: This needs to be reworked, as we move scale to be statically defined
	// array, this method can potentially take in a TEXTURE_ASSET_ID, depending on
	// how exactly we want to do collision
	return { abs(MapUtility::tile_size), abs(MapUtility::tile_size) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
// TODO: Add exact bounding box struct for each enemy
// And check if custom bounding boxes 
bool collides(const WorldPosition& position1, const WorldPosition& position2)
{
	vec2 dp = position1.position - position2.position;
	float dist_squared = dot(dp, dp);
	const vec2 other_bonding_box = get_bounding_box() / 2.f;
	const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	const vec2 my_bonding_box = get_bounding_box() / 2.f;
	const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	const float r_squared = max(other_r_squared, my_r_squared);
	return dist_squared < r_squared;
}

PhysicsSystem::PhysicsSystem(const Debug& debugging, std::shared_ptr<MapGeneratorSystem> map)
	: debugging(debugging), map_generator(std::move(map))
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
		WorldPosition& position_i = world_positions.components[i];
		uvec2 map_position_i = MapUtility::screen_position_to_map_position(position_i.position);

		// Check if the map position is occupy
		for (uint j = 0; j < map_positions.components.size(); j++) {
			MapPosition& map_position_j = map_positions.components[j];
			if (map_position_j.position == map_position_i) {
				// TODO: need to do collision more accurately, now we only
				// checks if the middle point of the projectile is within a tile
				Entity entity_j = map_positions.entities[j];
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}

		// Check if projectile hits a wall
		if (map_generator->is_wall(map_position_i) || !map_generator->is_on_map(map_position_i)) {
			registry.collisions.emplace_with_duplicates(entity_i, Entity::undefined());
		}
	}

	// you may need the following quantities to compute wall positions
	// (float)window_width_px; (float)window_height_px;

	// debugging of bounding boxes
	if (debugging.in_debug_mode) {
		// TODO: Do we need this anymore?
		uint size_before_adding_new = (uint)world_positions.components.size();
		for (uint i = 0; i < size_before_adding_new; i++) {
			WorldPosition& position_i = world_positions.components[i];
			// Entity entity_i = motion_container.entities[i];

			// visualize the radius with two axis-aligned lines
			const vec2 bonding_box = get_bounding_box();
			float radius = sqrt(dot(bonding_box / 2.f, bonding_box / 2.f));
			/*Entity line1 =*/create_line(position_i.position);
			/*Entity line2 =*/create_line(position_i.position);
		}
	}
}
