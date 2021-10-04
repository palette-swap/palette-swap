#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

Entity create_player(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { TILE_SIZE, TILE_SIZE };
	motion.scale.x *= 1; // Keep original orientation

	// Create and (empty) player component to be able to refer to other enttities
	registry.players.emplace(entity);
	registry.renderRequests.insert(entity,
								   { TEXTURE_ASSET_ID::PALADIN, // TEXTURE_COUNT indicates that no txture is needed
									 EFFECT_ASSET_ID::TEXTURED,
									 GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

// Repurposed into general create_enemy
// TODO: add additional inputs to specify enemy type, current default is slug
Entity create_enemy(RenderSystem* renderer, vec2 position)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// TODO: Add additional components associated with enemy instance
	// TODO: Change motion component based on grid system
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;

	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = mesh.original_size * 100.f;

	// TODO: Switch out basic enemy type based on input (Currently Defaulted to Slug)
	registry.renderRequests.insert(entity,
								   { TEXTURE_ASSET_ID::SLUG, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity create_line(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
		entity, { TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::LINE, GEOMETRY_BUFFER_ID::DEBUG_LINE });

	// Create motion
	Motion& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;
	motion.scale = scale;

	registry.debugComponents.emplace(entity);
	return entity;
}

// Creates a room entity, with room type referencing to the predefined room
Entity createRoom(RenderSystem* renderer, vec2 position, RoomType roomType)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& pos = registry.motions.emplace(entity);
	pos.position = position;
	pos.angle = 0.f;
	pos.scale = { TILE_SIZE * ROOM_SIZE, TILE_SIZE * ROOM_SIZE };

	Room& room = registry.rooms.emplace(entity);
	room.type = roomType;

	registry.renderRequests.insert(
		entity, { TEXTURE_ASSET_ID::WALKABLE1, EFFECT_ASSET_ID::TILE_MAP, GEOMETRY_BUFFER_ID::ROOM });

	return entity;
}

// (procedural) generate the maps
Entity generateMap(RenderSystem* renderer)
{
	auto entity = Entity();

	// We should only have a single map generator
	assert(registry.mapGenerator.size() == 0);

	MapGenerator& mapGenerator = registry.mapGenerator.emplace(entity);
	mapGenerator.generatorLevels();

	return entity;
}
