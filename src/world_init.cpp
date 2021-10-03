#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

Entity createPlayer(RenderSystem* renderer, vec2 pos)
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
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PALADIN, // TEXTURE_COUNT indicates that no txture is needed
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}


Entity createTurtle(RenderSystem* renderer, vec2 position)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the motion
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { -100.f, 0.f };
	motion.position = position;

	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ -TURTLE_BB_WIDTH, TURTLE_BB_HEIGHT });

	// Create and (empty) Turtle component to be able to refer to all turtles
	registry.hardShells.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TURTLE,
		 EFFECT_ASSET_ID::TEXTURED,
		 GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createLine(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
		 EFFECT_ASSET_ID::PEBBLE,
		 GEOMETRY_BUFFER_ID::DEBUG_LINE });

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

	Motion & pos = registry.motions.emplace(entity);
	pos.position = position;
	pos.angle = 0.f;
	pos.scale = { TILE_SIZE * ROOM_SIZE, TILE_SIZE * ROOM_SIZE };

	Room & room = registry.rooms.emplace(entity);
	room.type = roomType;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::WALKABLE1,
		 EFFECT_ASSET_ID::TILE_MAP,
		 GEOMETRY_BUFFER_ID::ROOM});

	return entity;
}

// (procedural) generate the maps
Entity generateMap(RenderSystem* renderer) {
	auto entity = Entity();

	// We should only have a single map generator
	assert(registry.mapGenerator.size() == 0);

	MapGenerator & mapGenerator = registry.mapGenerator.emplace(entity);
	mapGenerator.generatorLevels();

	return entity;
}
