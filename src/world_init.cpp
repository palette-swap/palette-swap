#include "world_init.hpp"


Entity create_player(uvec2 pos)
{
	auto entity = registry.create();

	// Create and (empty) player component to be able to refer to other enttities
	registry.emplace<Player>(entity);
	registry.emplace<MapPosition>(entity, pos);
	registry.emplace<Stats>(entity);

	Inventory& inventory = registry.emplace<Inventory>(entity);

	// Setup Casting
	Attack arcane_orb = { "Arcane Orb", 10, 10, 5, 10, DamageType::Magical, TargetingType::Projectile };
	Attack fireball = { "Fireball", -3, 10, 10, 20, DamageType::Fire, TargetingType::Projectile };
	inventory.inventory.emplace("Spellbook", create_weapon("Spellbook", std::vector<Attack>({ arcane_orb, fireball })));

	// Setup Sword
	Attack sword_light = { "Light", 4, 18, 12, 22, DamageType::Physical, TargetingType::Adjacent };
	Attack sword_heavy = { "Heavy", 1, 14, 20, 30, DamageType::Physical, TargetingType::Adjacent };
	Entity sword = create_weapon("Sword", std::vector<Attack>({ sword_light, sword_heavy }));
	inventory.inventory.emplace("Sword", sword);

	inventory.equipped.at(static_cast<uint8>(Slot::PrimaryHand)) = sword;

	registry.emplace<RenderRequest>(
		entity, TEXTURE_ASSET_ID::PALADIN, EFFECT_ASSET_ID::PLAYER, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);

	Animation& player_animation = registry.emplace<Animation>(entity);
	player_animation.max_frames = 6;
	player_animation.state = 0;
	player_animation.speed_adjustment = 1.5;

	registry.emplace<Color>(entity, vec3(1, 1, 1));
	
	return entity;
}

// Repurposed into general create_enemy
// TODO: add additional inputs to specify enemy type, current default is slug
// Note: Deprecated, use load_enemy from map_generator_system instead
Entity create_enemy(ColorState team, EnemyType type, uvec2 map_pos)
{
	auto entity = registry.create();

	registry.emplace<MapPosition>(entity, map_pos);

	// Set up enemy stats to be weaker than the player
	// TODO: Replace with load from file or auto-generate
	// TODO: Set different stats for each enemy type.
	Stats& stats = registry.emplace<Stats>(entity);
	stats.health = 50;
	stats.health_max = 50;
	stats.to_hit_bonus = 6;
	stats.evasion = 12;
	stats.base_attack.damage_min = 5;
	stats.base_attack.damage_max = 15;

	// Maps position of enemy to actual position (for reference)
	// vec2 actual_position = MapUtility::map_position_to_world_position(map_pos);

	// Indicates enemy is hittable by objects
	registry.emplace<Hittable>(entity);
	// Create enemy component for AI System.
	// TODO: Replace with load from file or auto-generate.
	Enemy& enemy = registry.emplace<Enemy>(entity);

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

	case EnemyType::Armor:
		enemy.radius = 2;
		enemy.speed = 1;
		enemy.attack_range = 1;
		break;

	case EnemyType::TreeAnt:
		enemy.radius = 3;
		enemy.speed = 1;
		enemy.attack_range = 2;
		break;

	case EnemyType::Wraith:
		enemy.radius = 1;
		enemy.speed = 1;
		enemy.attack_range = 1;
		break;

	default:
		throw std::runtime_error("Invalid enemy type.");
	}

	registry.emplace<RenderRequest>(
		entity, enemy_type_textures.at(static_cast<int>(type)), EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);
	if (team == ColorState::Red) {
		registry.emplace<Color>(entity, AnimationUtility::default_enemy_red);
	} else if (team == ColorState::Blue) {
		registry.emplace<Color>(entity, AnimationUtility::default_enemy_blue);
	} else {
		registry.emplace<Color>(entity, vec3(1, 1, 1));
	}


	Animation& enemy_animation = registry.emplace<Animation>(entity);
	enemy_animation.max_frames = 4;

	return entity;
}

Entity create_arrow(vec2 position)
{
	auto entity = registry.create();

	registry.emplace<WorldPosition>(entity, position);
	registry.emplace<Velocity>(entity, 0.f, 0.f);

	// Create and (empty) player component to be able to refer to other enttities
	// TODO: replace with single geometry buffer sprites
	registry.emplace<RenderRequest>(
		entity, TEXTURE_ASSET_ID::SPELLS, EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);
	Animation& spell_animation = registry.emplace<Animation>(entity);
	spell_animation.max_frames = 8;
	spell_animation.color = ColorState::None;
	spell_animation.state = 0;
	spell_animation.speed_adjustment = 1.5;
	registry.emplace<Color>(entity, vec3(1, 1, 1));

	return entity;
}

Entity create_line(vec2 position, float length, float angle)
{
	Entity entity = registry.create();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.emplace<Line>(entity, vec2(length, 2), angle);
	registry.emplace<Color>(entity, vec3(1, .1, .1));

	registry.emplace<WorldPosition>(entity, position);

	registry.emplace<DebugComponent>(entity);
	return entity;
}

Entity create_path_point(vec2 position)
{
	Entity entity = registry.create();

	registry.emplace<DebugComponent>(entity);

	registry.emplace<WorldPosition>(entity, position);

	// TODO: Replace CANNONBALL with other suitable texture.
	registry.emplace<RenderRequest>(
		entity, TEXTURE_ASSET_ID::CANNONBALL, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE, true);

	registry.emplace<Color>(entity, vec3(0, 1, 0));

	return entity;
}

Entity create_camera(uvec2 pos)
{
	auto entity = registry.create();

	// Setting initial position for camera
	registry.emplace<Camera>(entity);
	
	registry.emplace<WorldPosition>(entity, MapUtility::map_position_to_world_position(pos));

	return entity;
}

Entity create_team()
{
	Entity entity = registry.create();
	return entity;
}

Entity create_item(const std::string& name, const SlotList<bool>& allowed_slots)
{
	Entity entity = registry.create();
	registry.emplace<Item>(entity, name, 0.f, 0, allowed_slots);
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
	registry.emplace<Weapon>(entity, std::move(attacks));
	return entity;
}
