// internal
#include "ai_system.hpp"
#include "components.hpp"

// AI logic
AISystem::AISystem(std::shared_ptr<CombatSystem> combat,
				   std::shared_ptr<MapGeneratorSystem> map_generator,
				   std::shared_ptr<TurnSystem> turns)
	: combat(std::move(combat))
	, map_generator(std::move(map_generator))
	, turns(std::move(turns))
{
	registry.debug_components.emplace(enemy_team);

	this->turns->add_team_to_queue(enemy_team);
}

void AISystem::step(float /*elapsed_ms*/)
{
	if (!turns->execute_team_action(enemy_team)) {
		return;
	}

	for (long long i = registry.enemies.entities.size() - 1; i >= 0; i--) {
		const Entity& enemy_entity = registry.enemies.entities[i];

		if (remove_dead_entity(enemy_entity)) {
			continue;
		}

		const Enemy enemy = registry.enemies.components[i];

		ColorState active_world_color = turns->get_active_color();
		if (((uint8_t)active_world_color & (uint8_t)enemy.team) > 0) {
			switch (enemy.state) {

			case EnemyState::Idle:
				if (is_player_spotted(enemy_entity, 3)) {
					become_alert(enemy_entity);
					switch_enemy_state(enemy_entity, EnemyState::Active);
				}
				break;

			case EnemyState::Active:
				if (is_player_spotted(enemy_entity, 6)) {
					if (is_player_reachable(enemy_entity, 1)) {
						attack_player(enemy_entity);
					} else {
						approach_player(enemy_entity);
					}

					if (is_afraid(enemy_entity)) {
						switch_enemy_state(enemy_entity, EnemyState::Flinched);
					}
				} else {
					switch_enemy_state(enemy_entity, EnemyState::Idle);
				}
				break;

			case EnemyState::Flinched:
				if (is_at_nest(enemy_entity)) {
					switch_enemy_state(enemy_entity, EnemyState::Idle);
				} else {
					approach_nest(enemy_entity);
				}
				break;

			default:
				throw std::runtime_error("Invalid enemy state.");
			}
		}
	}

	turns->complete_team_action(enemy_team);
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

void AISystem::switch_enemy_state(const Entity& enemy_entity, EnemyState enemy_state)
{
	registry.enemies.get(enemy_entity).state = enemy_state;

	// TODO: Animate enemy state switch.
	TEXTURE_ASSET_ID& enemy_current_textrue = registry.render_requests.get(enemy_entity).used_texture;
	switch (enemy_state) {

	case EnemyState::Idle:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME;
		break;

	case EnemyState::Active:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_ALERT;
		break;

	case EnemyState::Flinched:
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_FLINCHED;
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

bool AISystem::is_player_reachable(const Entity& entity, const uint attack_range)
{
	return is_player_spotted(entity, attack_range);
}

bool AISystem::is_afraid(const Entity& entity)
{
	// TODO: Replace magic number 12 with variable amount
	if (registry.stats.get(entity).health < 12) {
		printf("Enemy is afraid...\n");
		return true;
	}
	return false;
}

bool AISystem::is_at_nest(const Entity& entity)
{
	assert(registry.enemy_nest_positions.has(entity));
	uvec2 nest_map_pos = registry.enemy_nest_positions.get(entity).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	return nest_map_pos == entity_map_pos;
}

void AISystem::become_alert(const Entity& /*entity*/)
{
	// TODO: Animate alert.
	printf("Enemy becomes alert!\n");
}

void AISystem::attack_player(const Entity& entity)
{
	// TODO: Animate attack.
	printf("Enemy attacks player!\n");
	Stats& stats = registry.stats.get(entity);
	Stats& player_stats = registry.stats.get(registry.players.top_entity());
	combat->do_attack(stats, stats.base_attack, player_stats);
}

bool AISystem::approach_player(const Entity& entity)
{
	uvec2 player_map_pos = registry.map_positions.get(registry.players.top_entity()).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, player_map_pos);
	if (shortest_path.size() > 2) {
		uvec2 next_map_pos = shortest_path[1];
		return move(entity, next_map_pos);
	}
	return false;
}

bool AISystem::approach_nest(const Entity& entity)
{
	assert(registry.enemy_nest_positions.has(entity));
	uvec2 nest_map_pos = registry.enemy_nest_positions.get(entity).position;
	uvec2 entity_map_pos = registry.map_positions.get(entity).position;

	std::vector<uvec2> shortest_path = map_generator->shortest_path(entity_map_pos, nest_map_pos);
	if (shortest_path.size() > 1) {
		uvec2 next_map_pos = shortest_path[1];
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
