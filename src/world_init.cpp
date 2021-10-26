#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

Entity create_player(uvec2 pos)
{
	auto entity = Entity();

	// Create and (empty) player component to be able to refer to other enttities
	registry.players.emplace(entity);
	registry.map_positions.emplace(entity, pos);
	registry.stats.emplace(entity);

	Inventory& inventory = registry.inventories.emplace(entity);

	// Setup Bow
	Attack bow_swipe = { "Swipe", 1, 15, 4, 10, DamageType::Physical, TargetingType::Adjacent };
	Attack bow_shoot = { "Shoot", 1, 20, 10, 25, DamageType::Physical, TargetingType::Projectile };
	inventory.inventory.emplace("Bow", create_weapon("Bow", std::vector<Attack>({ bow_shoot, bow_swipe })));

	// Setup Sword
	Attack sword_light = { "Light", 4, 18, 12, 22, DamageType::Physical, TargetingType::Adjacent };
	Attack sword_heavy = { "Heavy", 1, 14, 20, 30, DamageType::Physical, TargetingType::Adjacent };
	Entity sword = create_weapon("Sword", std::vector<Attack>({ sword_light, sword_heavy }));
	inventory.inventory.emplace("Sword", sword);

	inventory.equipped.at(static_cast<uint8>(Slot::PrimaryHand)) = sword;

	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::PALADIN, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });
	registry.colors.insert(entity, { 1, 1, 1 });

	return entity;
}

// Repurposed into general create_enemy
// TODO: add additional inputs to specify enemy type, current default is slug
Entity create_enemy(uvec2 position, ColorState team)
{
	auto entity = Entity();

	registry.map_positions.emplace(entity, position);

	// Set up enemy stats to be weaker than the player
	// TODO: Replace with load from file or auto-generate
	Stats& stats = registry.stats.emplace(entity);
	stats.health = 50;
	stats.health_max = 50;
	stats.to_hit_bonus = 6;
	stats.evasion = 12;
	stats.base_attack.damage_min = 5;
	stats.base_attack.damage_max = 15;

	// Maps position of enemy to actual position (for reference)
	vec2 actual_position = MapUtility::map_position_to_world_position(position);

	// Indicates enemy is hittable by objects
	registry.hittables.emplace(entity);

	// TODO: Switch out basic enemy type based on input (Currently Defaulted to Slug)
	registry.render_requests.insert(entity,
									{ TEXTURE_ASSET_ID::SLIME, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });
	registry.colors.insert(entity, { 1, 1, 1 });

	registry.enemy_states.emplace(entity, team);
	if (((uint8_t)team & 0b01) > 0) {
		registry.red_entities.emplace(entity);
	}
	if (((uint8_t)team & 0b10) > 0) {
		registry.blue_entities.emplace(entity);
	}
	registry.enemy_nest_positions.emplace(entity, position);

	return entity;
}

Entity create_arrow(vec2 position)
{
	auto entity = Entity();

	registry.world_positions.emplace(entity, position);
	registry.velocities.emplace(entity, 0.f, 0.f);

	// Create and (empty) player component to be able to refer to other enttities
	registry.render_requests.insert(entity,
									{ TEXTURE_ASSET_ID::ARROW, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity create_line(vec2 position)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::LINE, GEOMETRY_BUFFER_ID::DEBUG_LINE });

	registry.world_positions.emplace(entity, position);

	registry.debug_components.emplace(entity);
	return entity;
}

// Creates a room entity, with room type referencing to the predefined room
Entity create_room(vec2 position, MapUtility::RoomType roomType, float angle)
{
	auto entity = Entity();

	registry.world_positions.emplace(entity, position);
	registry.velocities.emplace(entity, 0, angle);

	Room& room = registry.rooms.emplace(entity);
	room.type = roomType;

	// TODO: Remove temporary workaround once #36 is resolved.
	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::TILE_SET, EFFECT_ASSET_ID::TILE_MAP, GEOMETRY_BUFFER_ID::ROOM });

	return entity;
}

Entity create_camera(vec2 /*pos*/, vec2 size, ivec2 central)
{
	auto entity = Entity();

	// Setting initial position for camera
	// MapPosition& map_position = registry.map_positions.emplace(entity, pos);
	Camera& camera = registry.cameras.emplace(entity);
	camera.size = size;
	camera.central = central;
	return entity;
}

Entity create_team()
{
	Entity entity = Entity();
	return entity;
}

Entity create_item(const std::string& name, SlotList<bool> allowed_slots)
{
	Entity entity = Entity();
	registry.items.insert(entity, { name, 0.f, 0, allowed_slots });
	return entity;
}

Entity create_weapon(const std::string& name, std::vector<Attack> attacks)
{
	const SlotList<bool> weapon_slots = [] {
		SlotList<bool> slots = { false };
		slots[static_cast<uint8>(Slot::PrimaryHand)] = true;
		return slots;
	}();
	Entity entity = create_item(name, weapon_slots);
	registry.weapons.emplace(entity, std::move(attacks));
	return entity;
}

Entity create_picture(vec2 pos)
{
	auto entity = Entity();

	// Create and (empty) player component to be able to refer to other enttities
	registry.world_positions.emplace(entity, pos);

	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::HELP_PIC, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });
	//registry.colors.insert(entity, { 1, 1, 1 });

	return entity;
}
