#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

Entity create_player(RenderSystem* renderer, uvec2 pos)
{
	auto entity = Entity();

	// Create and (empty) player component to be able to refer to other enttities
	registry.players.emplace(entity);
	registry.map_positions.emplace(entity, pos, vec2(tile_size, tile_size));

	vec2 actual_position = map_position_to_screen_position(pos);
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = actual_position;
	motion.scale = vec2(tile_size, tile_size);

	registry.render_requests.insert(entity,
								   { TEXTURE_ASSET_ID::PALADIN, 
									 EFFECT_ASSET_ID::TEXTURED,
									 GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

// Repurposed into general create_enemy
// TODO: add additional inputs to specify enemy type, current default is slug
Entity create_enemy(RenderSystem* renderer, uvec2 position)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)

	Mesh& mesh = renderer->get_mesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);


	registry.map_positions.emplace(entity, position, vec2(tile_size, tile_size));


	// Maps position of enemy to actual position (for reference)
	vec2 actual_position = map_position_to_screen_position(position);

	// TODO: Add additional components associated with enemy instance
	// TODO: Change motion component based on grid system
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = actual_position;

	// Setting initial values for enemy
	motion.scale = vec2(tile_size, tile_size);

	// Indicates enemy is hittable by objects
	registry.hittables.emplace(entity);

	// TODO: Switch out basic enemy type based on input (Currently Defaulted to Slug)
	registry.render_requests.insert(entity,
								   { TEXTURE_ASSET_ID::SLUG, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });

	registry.enemy_states.emplace(entity);
	registry.enemy_nest_positions.emplace(entity, position);

	return entity;
}

Entity create_arrow(RenderSystem* renderer, vec2 position)
{
	auto entity = Entity();

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = vec2(tile_size, tile_size) * 0.5f;

	// Create and (empty) player component to be able to refer to other enttities
	registry.render_requests.insert(
		entity,
		{ TEXTURE_ASSET_ID::ARROW, 
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity create_line(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::LINE, GEOMETRY_BUFFER_ID::DEBUG_LINE });

	// Create motion
	Motion& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;
	motion.scale = scale;

	registry.debug_components.emplace(entity);
	return entity;
}

// Creates a room entity, with room type referencing to the predefined room
Entity create_room(RenderSystem* renderer, vec2 position, RoomType roomType)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->get_mesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& pos = registry.motions.emplace(entity);
	pos.position = position;
	pos.angle = 0.f;
	pos.scale = { tile_size * room_size, tile_size * room_size };

	Room& room = registry.rooms.emplace(entity);
	room.type = roomType;

	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::TILE_MAP, GEOMETRY_BUFFER_ID::ROOM });

	return entity;
}
