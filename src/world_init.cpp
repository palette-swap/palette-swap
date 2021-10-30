#include "tiny_ecs_registry.hpp"
#include "world_init.hpp"


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

	registry.render_requests.insert(entity,
								   { TEXTURE_ASSET_ID::PALADIN, 
									 EFFECT_ASSET_ID::PLAYER,
									 GEOMETRY_BUFFER_ID::PLAYER });

	Animation & player_animation = registry.animations.emplace(entity);
	player_animation.max_frames = 6;
	player_animation.state = 0;
	player_animation.speed_adjustment = 1.5;

	registry.colors.insert(entity, { 1, 1, 1 });
	
	return entity;
}

// Repurposed into general create_enemy
// TODO: add additional inputs to specify enemy type, current default is slug
Entity create_enemy(ColorState team, EnemyType type, uvec2 map_pos)
{
	auto entity = Entity();

	registry.map_positions.emplace(entity, map_pos);

	// Set up enemy stats to be weaker than the player
	// TODO: Replace with load from file or auto-generate
	// TODO: Set different stats for each enemy type.
	Stats& stats = registry.stats.emplace(entity);
	stats.health = 50;
	stats.health_max = 50;
	stats.to_hit_bonus = 6;
	stats.evasion = 12;
	stats.base_attack.damage_min = 5;
	stats.base_attack.damage_max = 15;

	// Maps position of enemy to actual position (for reference)
	vec2 actual_position = MapUtility::map_position_to_world_position(map_pos);

	// Indicates enemy is hittable by objects
	registry.hittables.emplace(entity);

	// TODO: Switch out basic enemy type based on input (Currently Defaulted to Slug)
	registry.render_requests.insert(entity,
									{ TEXTURE_ASSET_ID::SLIME, EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::ENEMY });
	registry.colors.insert(entity, { 1, 1, 1 });

	// Create enemy component for AI System.
	// TODO: Replace with load from file or auto-generate.
	Enemy& enemy = registry.enemies.emplace(entity);

	enemy.team = team;
	enemy.type = type;
	enemy.state = EnemyState::Idle;
	enemy.nest_map_pos = map_pos;

	switch (type) {
	case EnemyType::Slime:
		enemy.radius = 3;
		enemy.speed = 1;
		enemy.attack_range = 1;
		break;

	case EnemyType::Raven:
		enemy.radius = 6;
		enemy.speed = 2;
		enemy.attack_range = 1 ;
		break;

	case EnemyType::LivingArmor:
		enemy.radius = 2;
		enemy.speed = 1;
		enemy.attack_range = 1;
		break;

	case EnemyType::TreeAnt:
		enemy.radius = 3;
		enemy.speed = 1;
		enemy.attack_range = 2;
		break;

	default:
		throw std::runtime_error("Invalid enemy type.");
	}


	// TODO: Combine with render_requests above, so animation system handles render requests as a middleman
	// Update animation number using animation system max frames, instead of this static number
	Animation& enemy_animation = registry.animations.emplace(entity);
	enemy_animation.max_frames = 4;

	return entity;
}

Entity create_arrow(vec2 position)
{
	auto entity = Entity();

	registry.world_positions.emplace(entity, position);
	registry.velocities.emplace(entity, 0.f, 0.f);

	// Create and (empty) player component to be able to refer to other enttities
	registry.render_requests.insert(
		entity,
		{ TEXTURE_ASSET_ID::CANNONBALL, 
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });
	registry.colors.insert(entity, { 1, 1, 1 });

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

Entity create_path_point(vec2 position)
{
	Entity entity = Entity();

	registry.debug_components.emplace(entity);

	registry.world_positions.emplace(entity, position);

	// TODO: Replace CANNONBALL with other suitable texture.
	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::CANNONBALL, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE });

	registry.colors.insert(entity, { 0, 1, 0 });

	return entity;
}

// Creates a room entity, with room type referencing to the predefined room
Entity create_room(vec2 position, MapUtility::RoomType roomType)
{
	auto entity = Entity();

	registry.world_positions.emplace(entity, position);

	Room& room = registry.rooms.emplace(entity);
	room.type = roomType;

	// TODO: Remove temporary workaround once #36 is resolved.
	registry.render_requests.insert(
		entity, { TEXTURE_ASSET_ID::TILE_SET, EFFECT_ASSET_ID::TILE_MAP, GEOMETRY_BUFFER_ID::ROOM });

	return entity;
}

Entity create_camera(uvec2 pos)
{
	auto entity = Entity();

	// Setting initial position for camera
	Camera& camera = registry.cameras.emplace(entity);
	
	registry.map_positions.emplace(entity, pos);

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
