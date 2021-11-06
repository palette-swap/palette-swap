// internal
#include "ai_system.hpp"
#include "components.hpp"
#include "world_init.hpp"

// AI logic
AISystem::AISystem(const Debug& debugging,
				   std::shared_ptr<CombatSystem> combat,
				   std::shared_ptr<MapGeneratorSystem> map_generator,
				   std::shared_ptr<TurnSystem> turns,
				   std::shared_ptr<AnimationSystem> animations)
	: debugging(debugging)
	, combat(std::move(combat))
	, map_generator(std::move(map_generator))
	, turns(std::move(turns))
	, animations(std::move(animations))
	, enemy_team(registry.create())
{
	registry.emplace<DebugComponent>(enemy_team);

	this->turns->add_team_to_queue(enemy_team);

	enemy_attack1_wav.load(audio_path("enemy_attack1.wav").c_str());
	std::vector<std::function<void(const Entity& attacker, const Entity& target)>> callbacks;

	this->combat->attach_do_attack_callback(
		[this](const Entity& attacker, const Entity& target) { this->do_attack_callback(attacker, target); });
}

void AISystem::step(float /*elapsed_ms*/)
{
	if (turns->execute_team_action(enemy_team)) {
		for (auto [enemy_entity, enemy] : registry.view<Enemy>().each()) {

			if (remove_dead_entity(enemy_entity)) {
				continue;
			}

			ColorState active_world_color = turns->get_active_color();
			if (((uint8_t)active_world_color & (uint8_t)enemy.team) > 0) {

				switch (enemy.type) {
				case EnemyType::Slime:
					execute_slime(enemy_entity);
					break;

				case EnemyType::Raven:
				case EnemyType::Wraith:
					execute_raven(enemy_entity);
					break;

				case EnemyType::Armor:
					execute_armor(enemy_entity);
					break;

				case EnemyType::TreeAnt:
					execute_treeant(enemy_entity);
					break;

				default:
					throw std::runtime_error("Invalid enemy type.");
				}
			}
		}

		turns->complete_team_action(enemy_team);
	}

	// Debugging for the shortest paths of enemies.
	if (debugging.in_debug_mode) {
		for (auto [enemy_entity, enemy, entity_map_position] : registry.view<Enemy, MapPosition>().each()) {

			ColorState active_world_color = turns->get_active_color();
			if (((uint8_t)active_world_color & (uint8_t)enemy.team) > 0) {
				const uvec2& entity_map_pos = entity_map_position.position;

				if (enemy.state == EnemyState::Flinched) {
					const uvec2& nest_map_pos = enemy.nest_map_pos;
					std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, nest_map_pos);

					for (const uvec2& path_point : shortest_path) {
						create_path_point(MapUtility::map_position_to_world_position(path_point));
					}
				} else if (enemy.state != EnemyState::Idle && is_player_spotted(enemy_entity, enemy.radius * 2)) {
					Entity player = registry.view<Player>().front();
					std::vector<uvec2> shortest_path
						= map_generator->shortest_path(entity_map_pos, registry.get<MapPosition>(player).position);

					for (const uvec2& path_point : shortest_path) {
						create_path_point(MapUtility::map_position_to_world_position(path_point));
					}
				}
			}
		}
	}
}

void AISystem::do_attack_callback(const Entity& attacker, const Entity& target)
{
	if (registry.any_of<Player>(attacker)) {
		Enemy& enemy = registry.get<Enemy>(target);
		if ((enemy.state == EnemyState::Idle && !is_player_spotted(target, enemy.radius))
			|| (enemy.state != EnemyState::Idle && !is_player_spotted(target, enemy.radius * 2))) {
			enemy.radius = MapUtility::room_size * MapUtility::map_size;
		}
	}
}

void AISystem::execute_slime(const Entity& slime)
{
	Enemy& enemy = registry.get<Enemy>(slime);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(slime, enemy.radius)) {
			switch_enemy_state(slime, EnemyState::Active);
			execute_slime(slime);
			return;
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
			enemy.radius = 3;
			if (!is_player_spotted(slime, enemy.radius * 2)) {
				recover_health(slime, 1.f);
				switch_enemy_state(slime, EnemyState::Idle);
			}
		} else {
			approach_nest(slime, enemy.speed);
		}
		break;

	default:
		throw std::runtime_error("Invalid enemy state of Slime.");
	}
}

void AISystem::execute_raven(const Entity& raven)
{
	const Enemy& enemy = registry.get<Enemy>(raven);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(raven, enemy.radius)) {
			switch_enemy_state(raven, EnemyState::Active);
			execute_raven(raven);
			return;
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

void AISystem::execute_armor(const Entity& living_armor)
{
	const Enemy& enemy = registry.get<Enemy>(living_armor);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(living_armor, enemy.radius)) {
			switch_enemy_state(living_armor, EnemyState::Active);
			execute_armor(living_armor);
			return;
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
			switch_enemy_state(living_armor, EnemyState::Idle);
		}
		break;

	case EnemyState::Immortal:
		become_immortal(living_armor, false);
		switch_enemy_state(living_armor, EnemyState::Active);
		break;

	default:
		throw std::runtime_error("Invalid enemy state of Armor.");
	}
}

void AISystem::execute_treeant(const Entity& tree_ant)
{
	const Enemy& enemy = registry.get<Enemy>(tree_ant);

	switch (enemy.state) {

	case EnemyState::Idle:
		if (is_player_spotted(tree_ant, enemy.radius)) {
			switch_enemy_state(tree_ant, EnemyState::Active);
			execute_treeant(tree_ant);
			return;
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
			switch_enemy_state(tree_ant, EnemyState::Idle);
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
		throw std::runtime_error("Invalid enemy state of Armor.");
	}
}

bool AISystem::remove_dead_entity(const Entity& entity)
{
	// TODO: Animate death.
	if (registry.any_of<Stats>(entity) && registry.get<Stats>(entity).health <= 0) {
		registry.destroy(entity);
		return true;
	}
	return false;
}

void AISystem::switch_enemy_state(const Entity& enemy_entity, EnemyState new_state)
{
	Enemy& enemy = registry.get<Enemy>(enemy_entity);
	enemy.state = new_state;

	if (enemy_state_to_animation_state.count(enemy.state) == 1) {
		int new_state = enemy_state_to_animation_state[enemy.state];
		animations->set_enemy_state(enemy_entity, new_state);
	} else {
		throw std::runtime_error("Invalid enemy state.");
	}
}

bool AISystem::is_player_spotted(const Entity& entity, const uint radius)
{
	uvec2 player_map_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	uvec2 entity_map_pos = registry.get<MapPosition>(entity).position;

	ivec2 distance = entity_map_pos - player_map_pos;
	return uint(abs(distance.x)) <= radius && uint(abs(distance.y)) <= radius;
}

bool AISystem::is_player_in_attack_range(const Entity& entity, const uint attack_range)
{
	return is_player_spotted(entity, attack_range);
}

bool AISystem::is_at_nest(const Entity& entity)
{
	const uvec2& entity_map_pos = registry.get<MapPosition>(entity).position;
	const uvec2& nest_map_pos = registry.get<Enemy>(entity).nest_map_pos;

	return entity_map_pos == nest_map_pos;
}

void AISystem::attack_player(const Entity& entity)
{
	Entity player = registry.view<Player>().front();
	combat->do_attack(entity, registry.get<Stats>(entity).base_attack, player);

	// TODO: temporarily for debugging in console, remove it later.
	char* enemy_type = enemy_type_to_string[registry.get<Enemy>(entity).type];
	printf("%s_%u attacks player!\n", enemy_type, (uint)entity);

	so_loud.play(enemy_attack1_wav);
}

bool AISystem::approach_player(const Entity& entity, uint speed)
{
	const uvec2 player_map_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	const uvec2 entity_map_pos = registry.get<MapPosition>(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, player_map_pos);
	if (shortest_path.size() > 2) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 2))];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::approach_nest(const Entity& entity, uint speed)
{
	const uvec2 player_map_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	const uvec2& entity_map_pos = registry.get<MapPosition>(entity).position;
	const uvec2& nest_map_pos = registry.get<Enemy>(entity).nest_map_pos;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, nest_map_pos);
	if (shortest_path.size() > 1) {
		uvec2 next_map_pos = shortest_path[min((size_t)speed, (shortest_path.size() - 1))];
		// A special case that the player occupies the nest, so the entity won't move in (overlap).
		if (next_map_pos == nest_map_pos && nest_map_pos == player_map_pos) {
			return false;
		}
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::move(const Entity& entity, const uvec2& map_pos)
{
	MapPosition& entity_map_pos = registry.get<MapPosition>(entity);
	if (entity_map_pos.position != map_pos && map_generator->walkable(map_pos)) {
		if (map_pos.x < entity_map_pos.position.x) {
			animations->set_sprite_direction(entity, Sprite_Direction::SPRITE_LEFT);
		} else if (map_pos.x > entity_map_pos.position.x) {
			animations->set_sprite_direction(entity, Sprite_Direction::SPRITE_RIGHT);
		}
		entity_map_pos.position = map_pos;
		return true;
	}
	return false;
}

void AISystem::recover_health(const Entity& entity, float ratio)
{
	Stats& stats = registry.get<Stats>(entity);
	stats.health += static_cast<int>(static_cast<float>(stats.health_max) * ratio);
	stats.health = min(stats.health, stats.health_max);
}

bool AISystem::is_health_below(const Entity& entity, float ratio)
{
	const Stats& stats = registry.get<Stats>(entity);
	return static_cast<float>(stats.health) < static_cast<float>(stats.health_max) * ratio;
}

void AISystem::become_immortal(const Entity& entity, bool flag)
{
	Stats& stats = registry.get<Stats>(entity);
	for (int& damage_modifier : stats.damage_modifiers) {
		damage_modifier = flag ? INT_MIN : 0;
	}
}

void AISystem::become_powerup(const Entity& entity, bool flag)
{
	Stats& stats = registry.get<Stats>(entity);
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
