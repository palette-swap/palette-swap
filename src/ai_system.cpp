// internal
#include "ai_system.hpp"
#include "components.hpp"

// AI logic
AISystem::AISystem(std::shared_ptr<CombatSystem> combat,
				   std::shared_ptr<MapGeneratorSystem> map_generator,
				   std::shared_ptr<TurnSystem> turns,
				   std::shared_ptr<AnimationSystem> animations)
	: combat(std::move(combat))
	, map_generator(std::move(map_generator))
	, turns(std::move(turns))
	, animations(std::move(animations))
{
	registry.debug_components.emplace(enemy_team);

	this->turns->add_team_to_queue(enemy_team);
}

void AISystem::step(float /*elapsed_ms*/)
{
	if (!turns->execute_team_action(enemy_team)) {
		return;
	}

	for (long long i = registry.enemies.entities.size() - 1; i >= 0; --i) {
		const Entity& enemy_entity = registry.enemies.entities[i];

		if (remove_dead_entity(enemy_entity)) {
			continue;
		}

		const Enemy enemy = registry.enemies.components[i];

		ColorState active_world_color = turns->get_active_color();
		if (((uint8_t)active_world_color & (uint8_t)enemy.team) > 0) {

			switch (enemy.type) {
			case EnemyType::Slime:
				execute_Slime(enemy_entity);
				break;

			case EnemyType::Raven:
				execute_Raven(enemy_entity);
				break;

			case EnemyType::LivingArmor:
				execute_LivingArmor(enemy_entity);
				break;

			case EnemyType::TreeAnt:
				execute_TreeAnt(enemy_entity);
				break;

			default:
				throw std::runtime_error("Invalid enemy type.");
			}
		}
	}

	turns->complete_team_action(enemy_team);
}

void AISystem::execute_Slime(const Entity& slime)
{
	const Enemy& enemy = registry.enemies.get(slime);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(slime, enemy.radius)) {
			switch_enemy_state(slime, EnemyState::Active);
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(slime, enemy.radius * 2)) {
			if (is_player_in_attack_range(slime, enemy.attack_range)) {
				attack_player(slime);
			} else {
				approach_player(slime, enemy.speed);
			}

			if (is_health_below(slime, 0.25f)) {
				switch_enemy_state(slime, EnemyState::Flinched);
			}
		} else {
			switch_enemy_state(slime, EnemyState::Idle);
		}
		break;

	case EnemyState::Flinched:
		if (is_at_nest(slime)) {
			switch_enemy_state(slime, EnemyState::Idle);
		} else {
			approach_nest(slime, enemy.speed);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state of Slime.");
	}
}

void AISystem::execute_Raven(const Entity& raven)
{
	const Enemy& enemy = registry.enemies.get(raven);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(raven, enemy.radius)) {
			switch_enemy_state(raven, EnemyState::Active);
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(raven, enemy.radius * 2)) {
			if (is_player_in_attack_range(raven, enemy.attack_range)) {
				attack_player(raven);
			} else {
				approach_player(raven, enemy.speed);
			}
		} else {
			switch_enemy_state(raven, EnemyState::Idle);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state of Raven.");
	}
}

void AISystem::execute_LivingArmor(const Entity& living_armor)
{
	const Enemy& enemy = registry.enemies.get(living_armor);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(living_armor, enemy.radius)) {
			switch_enemy_state(living_armor, EnemyState::Active);
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(living_armor, enemy.radius * 2)) {
			if (is_player_in_attack_range(living_armor, enemy.attack_range)) {
				attack_player(living_armor);
			} else {
				approach_player(living_armor, enemy.speed);
			}

			if (uniform_dist(rng) < 0.20f) {
				become_immortal(living_armor, true);
				switch_enemy_state(living_armor, EnemyState::Immortal);
			}
		} else {
			if (is_at_nest(living_armor)) {
				switch_enemy_state(living_armor, EnemyState::Idle);
			} else {
				approach_nest(living_armor, enemy.speed);
			}
		}
		break;

	case EnemyState::Immortal:
		become_immortal(living_armor, false);
		switch_enemy_state(living_armor, EnemyState::Active);
		break;

	default:
		throw std::runtime_error("Invalid enemy state of LivingArmor.");
	}
}

void AISystem::execute_TreeAnt(const Entity& tree_ant)
{
	const Enemy& enemy = registry.enemies.get(tree_ant);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(tree_ant, enemy.radius)) {
			switch_enemy_state(tree_ant, EnemyState::Active);
		}
		break;

	case EnemyState::Active:
		if (is_player_spotted(tree_ant, enemy.radius * 2)) {
			if (is_player_in_attack_range(tree_ant, enemy.attack_range)) {
				attack_player(tree_ant);
			} else {
				approach_player(tree_ant, enemy.speed);
			}

			if (is_health_below(tree_ant, 0.20f)) {
				become_powerup(tree_ant, true);
				switch_enemy_state(tree_ant, EnemyState::Powerup);
			}
		} else {
			if (is_at_nest(tree_ant)) {
				switch_enemy_state(tree_ant, EnemyState::Idle);
			} else {
				approach_nest(tree_ant, enemy.speed);
			}
		}
		break;

	case EnemyState::Powerup:
		if (is_player_spotted(tree_ant, enemy.radius * 2)) {
			if (is_player_in_attack_range(tree_ant, enemy.attack_range * 2)) {
				attack_player(tree_ant);
			}
			// TreeAnt can't move when it's powered up.
		} else {
			become_powerup(tree_ant, false);
			switch_enemy_state(tree_ant, EnemyState::Active);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state of LivingArmor.");
	}
}

bool AISystem::remove_dead_entity(const Entity& entity)
{
	// TODO: Animate death.
	if (registry.stats.has(entity) && registry.stats.get(entity).health <= 0) {
		registry.remove_all_components_of(entity);
		return true;
	}
	return false;
}

void AISystem::switch_enemy_state(const Entity& enemy_entity, EnemyState new_state)
{
	
	Enemy& enemy = registry.enemies.get(enemy_entity);
	enemy.state = new_state;
	Animation& enemy_animation = registry.animations.get(enemy_entity);

	// TODO: Animate enemy state switch by a spreadsheet for enemy_type * enemy_state mapping.
	// Slime:		Idle, Active, Flinched.
	// Raven:		Idle, Active.
	// LivingArmor:	Idle, Active, Powerup.
	// TreeAnt:		Idle, Active, Immortal.
	
	//Animation& animation = registry.animations.get(enemy_entity);
	//animation.switchTexture(enemy.team, enemy.type, enemy.state);

	switch (enemy.state) {

	case EnemyState::Idle:
		enemy_animation.state = 0;
		break;

	case EnemyState::Active:
		enemy_animation.state = 1;
		break;

	case EnemyState::Flinched:
		enemy_animation.state = 2;
		break;

	case EnemyState::Powerup:
		enemy_animation.state = 2;
		break;

	case EnemyState::Immortal:
		enemy_animation.state = 2;
		break;

	default:
		throw std::runtime_error("Invalid enemy state.");
	}
}

bool AISystem::is_player_spotted(const Entity& entity, const uint radius)
{
	uvec2 player_map_pos = registry.map_positions.get(registry.players.top_entity()).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	ivec2 distance = entity_map_pos - player_map_pos;
	return uint(abs(distance.x)) <= radius && uint(abs(distance.y)) <= radius;
}

bool AISystem::is_player_in_attack_range(const Entity& entity, const uint attack_range)
{
	return is_player_spotted(entity, attack_range);
}

bool AISystem::is_at_nest(const Entity& entity)
{
	const uvec2& entity_map_pos = registry.map_positions.get(entity).position;
	const uvec2& nest_map_pos = registry.enemies.get(entity).nest_map_pos;

	return entity_map_pos == nest_map_pos;
}

void AISystem::attack_player(const Entity& entity)
{
	// TODO: Animate attack.
	char* enemy_type = enemy_type_to_string[registry.enemies.get(entity).type];
	printf("%s_%u attacks player!\n", enemy_type, (uint)entity);
	
	Stats& entity_stats = registry.stats.get(entity);
	Stats& player_stats = registry.stats.get(registry.players.top_entity());
	// TODO: move attack animation to combat system potentially
	// Triggers attack for a enemy entity
	animations->enemy_attack_animation(entity);
	combat->do_attack(entity_stats, entity_stats.base_attack, player_stats);
}

bool AISystem::approach_player(const Entity& entity, uint speed)
{
	uvec2 player_map_pos = registry.map_positions.get(registry.players.top_entity()).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, player_map_pos);
	if (shortest_path.size() > 2) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 2))];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::approach_nest(const Entity& entity, uint speed)
{
	const uvec2& entity_map_pos = registry.map_positions.get(entity).position;
	const uvec2& nest_map_pos = registry.enemies.get(entity).nest_map_pos;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, nest_map_pos);
	if (shortest_path.size() > 1) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 1))];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::move(const Entity& entity, const uvec2& map_pos)
{
	// TODO: Animate move.
	MapPosition& entity_map_pos = registry.map_positions.get(entity);
	if (entity_map_pos.position != map_pos && map_generator->walkable(map_pos)) {
		entity_map_pos.position = map_pos;
		return true;
	}
	return false;
}

bool AISystem::is_health_below(const Entity& entity, float ratio)
{
	const Stats& states = registry.stats.get(entity);
	return static_cast<float>(states.health) < static_cast<float>(states.health_max) * ratio;
}

void AISystem::become_immortal(const Entity& entity, bool flag)
{
	Stats& stats = registry.stats.get(entity);
	for (int& damage_modifier : stats.damage_modifiers) {
		damage_modifier = flag ? INT_MIN : 0;
	}
}

void AISystem::become_powerup(const Entity& entity, bool flag)
{
	Stats& stats = registry.stats.get(entity);
	if (flag) {
		stats.to_hit_bonus *= 2;
		stats.base_attack.to_hit_min *= 2;
		stats.base_attack.to_hit_max *= 2;
		stats.damage_bonus *= 2;
		stats.base_attack.damage_min *= 2;
		stats.base_attack.damage_max *= 2;
	} else {
		stats.to_hit_bonus /= 2;
		stats.base_attack.to_hit_min /= 2;
		stats.base_attack.to_hit_max /= 2;
		stats.damage_bonus /= 2;
		stats.base_attack.damage_min /= 2;
		stats.base_attack.damage_max /= 2;
	}
}
