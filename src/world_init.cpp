#include "world_init.hpp"

Entity create_player(uvec2 pos)
{
	auto entity = registry.create();

	// Create and (empty) player component to be able to refer to other enttities
	registry.emplace<Player>(entity);
	registry.emplace<MapPosition>(entity, pos);
	registry.emplace<Stats>(entity);
	registry.emplace<PlayerStats>(entity);

	// Light up around the player
	registry.emplace<Light>(entity, MapUtility::tile_size * 7.5f);

	Inventory& inventory = registry.emplace<Inventory>(entity);

	// Setup Casting
	Entity fireball_entity = registry.create();
	Attack& fireball = registry.emplace<Attack>(fireball_entity, "Fireball", -3, 13, 10, 20, DamageType::Fire, TargetingType::Projectile, -1);
	fireball.mana_cost = 25;
	fireball.parallel_size = 4;
	fireball.perpendicular_size = 4;
	Entity burn_entity = registry.create();
	EffectEntry& burn = registry.emplace<EffectEntry>(burn_entity, entt::null, Effect::Burn, .95f, 5);
	fireball.effects = burn_entity;
	inventory.equipped.at(static_cast<uint8>(Slot::Spell1))
		= create_spell("Fireball", std::vector<Entity>({ fireball_entity }));
	registry.get<ItemTemplate>(inventory.equipped.at(static_cast<uint8>(Slot::Spell1))).texture_offset = ivec2(0, 3);

	// Setup Sword
	Entity light_entity = registry.create();
	registry.emplace<Attack>(light_entity, "Slice", 4, 18, 8, 18, DamageType::Physical, TargetingType::Adjacent);
	Entity heavy_entity = registry.create();
	registry.emplace<Attack>(heavy_entity, "Stab", 1, 14, 15, 25, DamageType::Physical, TargetingType::Adjacent);
	inventory.equipped.at(static_cast<uint8>(Slot::Weapon))
		= create_weapon("Sword", std::vector<Entity>({ light_entity, heavy_entity }));

	registry.emplace<RenderRequest>(
		entity, TEXTURE_ASSET_ID::PALADIN, EFFECT_ASSET_ID::PLAYER, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);

	Animation& player_animation = registry.emplace<Animation>(entity);
	player_animation.max_frames = 6;
	player_animation.state = 0;
	player_animation.speed_adjustment = 0.5f;

	registry.emplace<Color>(entity, vec3(1, 1, 1));
	registry.emplace<PlayerInactivePerception>(entity);
	
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
	stats.to_hit_weapons = stats.to_hit_spells = 6;
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
	enemy.behaviour = enemy_type_to_behaviour.at(static_cast<int>(enemy.type));
	enemy.state = EnemyState::Idle;
	enemy.nest_map_pos = map_pos;

	switch (type) {

	case EnemyType::TrainingDummy:
		enemy.radius = 0;
		enemy.speed = 0;
		enemy.attack_range = 0;
		break;

	case EnemyType::Slime:
		enemy.radius = 3;
		enemy.speed = 1;
		enemy.attack_range = 1;
		break;

	case EnemyType::Raven:
		enemy.radius = 6;
		enemy.speed = 2;
		enemy.attack_range = 1;
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

	case EnemyType::Mushroom:
		enemy.radius = 10;
		enemy.speed = 1;
		enemy.attack_range = 1;
		break;

	case EnemyType::KoboldMage:
		enemy.radius = 10;
		enemy.speed = 1;
		enemy.attack_range = 1;
		break;

	case EnemyType::KingMush:
		enemy.radius = 10;
		enemy.speed = 0;
		enemy.attack_range = 10;
		break;

	case EnemyType::Dragon:
		enemy.radius = 10;
		enemy.speed = 0;
		enemy.attack_range = 3;
		break;

	case EnemyType::AOERingGen:
		enemy.radius = 0;
		enemy.speed = 0;
		enemy.attack_range = 0;
		break;

	default:
		throw std::runtime_error("Invalid enemy type.");
	}

	Animation& enemy_animation = registry.emplace<Animation>(entity);
	enemy_animation.max_frames = 4;

	AnimationProfile enemy_profile = enemy_type_to_animation_profile.at(static_cast<int>(type));
	enemy_animation.travel_offset = enemy_profile.travel_offset;

	registry.emplace<RenderRequest>(
		entity, enemy_profile.texture, EFFECT_ASSET_ID::ENEMY, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);
	if (team == ColorState::Red) {
		enemy_animation.display_color = vec4(AnimationUtility::default_enemy_red,1);
		registry.emplace<RedExclusive>(entity);
	} else if (team == ColorState::Blue) {
		enemy_animation.display_color = vec4(AnimationUtility::default_enemy_blue,1);
		registry.emplace<BlueExclusive>(entity);
	}

	return entity;
}

Entity create_aoe_emitter(ColorState team, uvec2 map_pos) 
{
	auto entity = registry.create();

	registry.emplace<MapPosition>(entity, map_pos);

	// Set up enemy stats to be weaker than the player
	Stats& stats = registry.emplace<Stats>(entity);
	stats.health = 50;
	stats.health_max = 50;
	stats.to_hit_weapons = stats.to_hit_spells = 6;
	stats.evasion = 12;
	stats.base_attack.damage_min = 5;
	stats.base_attack.damage_max = 15;

	Enemy& enemy = registry.emplace<Enemy>(entity);

	enemy.team = team;
	enemy.type = EnemyType::AOERingGen;
	enemy.behaviour = EnemyBehaviour::AOERingGen;
	enemy.state = EnemyState::Active;
	enemy.nest_map_pos = map_pos;
	enemy.radius = 0;
	enemy.speed = 0;
	enemy.attack_range = 0;

	return entity;
}

Entity create_guide( uvec2 map_pos)
{
	auto entity = registry.create();

	registry.emplace<Guide>(entity);
	registry.emplace<MapPosition>(entity, map_pos);

	// Sets animation and display colours for the guide. Currently makes her transparent for "narrative" elements
	Animation& guide_animation = registry.emplace<Animation>(entity);
	guide_animation.max_frames = 6;
	guide_animation.speed_adjustment = 0.5f;
	guide_animation.display_color.w = 0.6f;


	registry.emplace<RenderRequest>(entity,
									TEXTURE_ASSET_ID::GUIDE,
									EFFECT_ASSET_ID::ENEMY,
									GEOMETRY_BUFFER_ID::SMALL_SPRITE,
									true);
	return entity;
}


std::vector<Entity> create_aoe(const std::vector<uvec2>& aoe_area, const Stats& stats, EnemyType enemy_type, Entity owner)
{
	std::vector<Entity> aoe;

	for (const uvec2& map_pos : aoe_area) {
		Entity aoe_square = AOESource::add(owner);

		registry.emplace<WorldPosition>(aoe_square, MapUtility::map_position_to_world_position(map_pos));

		registry.emplace<Stats>(aoe_square, stats);

		registry.emplace<EffectRenderRequest>(
			aoe_square, boss_type_attack_spritesheet.at(enemy_type), EFFECT_ASSET_ID::AOE, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);

		registry.emplace<Animation>(aoe_square);

		registry.emplace<Color>(aoe_square, vec3(1, 0, 0));

		aoe.push_back(aoe_square);
	}

	return aoe;
}

Entity create_arrow(vec2 position)
{
	auto entity = registry.create();

	registry.emplace<WorldPosition>(entity, position);
	registry.emplace<Velocity>(entity, 0.f, 0.f);

	// Create and (empty) player component to be able to refer to other entities
	// TODO: replace with single geometry buffer sprites
	registry.emplace<EffectRenderRequest>(
		entity, TEXTURE_ASSET_ID::SPELLS, EFFECT_ASSET_ID::SPELL, GEOMETRY_BUFFER_ID::SMALL_SPRITE, true);
	Animation& spell_animation = registry.emplace<Animation>(entity);
	spell_animation.max_frames = 8;
	spell_animation.color = ColorState::None;
	spell_animation.state = 0;
	spell_animation.speed_adjustment = 1;
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

Entity create_item_template(const std::string& name, const SlotList<bool>& allowed_slots)
{
	Entity entity = registry.create();
	registry.emplace<ItemTemplate>(entity, name, 0, allowed_slots);
	return entity;
}

Entity create_spell(const std::string& name, std::vector<Entity> attacks)
{
	const SlotList<bool> spell_slots = [] {
		SlotList<bool> slots = { false };
		slots[static_cast<uint8>(Slot::Spell1)] = true;
		slots[static_cast<uint8>(Slot::Spell2)] = true;
		return slots;
	}();
	Entity entity = create_item_template(name, spell_slots);
	registry.emplace<Weapon>(entity).given_attacks.swap(attacks);
	return entity;
}

Entity create_weapon(const std::string& name, std::vector<Entity> attacks)
{
	const SlotList<bool> weapon_slots = [] {
		SlotList<bool> slots = { false };
		slots[static_cast<uint8>(Slot::Weapon)] = true;
		return slots;
	}();
	Entity entity = create_item_template(name, weapon_slots);
	registry.emplace<Weapon>(entity).given_attacks.swap(attacks);
	return entity;
}