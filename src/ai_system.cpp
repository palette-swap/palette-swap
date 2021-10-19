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

	for (long long i = registry.enemy_states.entities.size() - 1; i >= 0; i--) {
		const Entity& enemy_entity = registry.enemy_states.entities[i];

		// TODO: Animate death instead of just removing
		if (registry.stats.has(enemy_entity) && registry.stats.get(enemy_entity).health <= 0) {
			registry.remove_all_components_of(enemy_entity);
			continue;
		}

		switch (registry.enemy_states.get(enemy_entity).current_state) {

		case ENEMY_STATE_ID::Idle:
			if (is_player_spotted(enemy_entity, 3)) {
				become_alert(enemy_entity);
				switch_enemy_state(enemy_entity, ENEMY_STATE_ID::ACTIVE);
			}
			break;

		case ENEMY_STATE_ID::ACTIVE:
			if (is_player_spotted(enemy_entity, 6)) {
				if (is_player_reachable(enemy_entity, 1)) {
					attack_player(enemy_entity);
				} else {
					approach_player(enemy_entity);
				}

				if (is_afraid(enemy_entity)) {
					switch_enemy_state(enemy_entity, ENEMY_STATE_ID::FLINCHED);
				}
			} else {
				switch_enemy_state(enemy_entity, ENEMY_STATE_ID::Idle);
			}
			break;

		case ENEMY_STATE_ID::FLINCHED:
			if (is_at_nest(enemy_entity)) {
				switch_enemy_state(enemy_entity, ENEMY_STATE_ID::Idle);
			} else {
				approach_nest(enemy_entity);
			}
			break;

		default:
			throw std::runtime_error("Invalid Enemy State");
		}
	}

	turns->complete_team_action(enemy_team);
}

void AISystem::switch_enemy_state(const Entity& enemy_entity, ENEMY_STATE_ID enemy_state)
{
	ENEMY_STATE_ID& enemy_current_state = registry.enemy_states.get(enemy_entity).current_state;
	Animation& animation = registry.animations.get(enemy_entity);

	// Keep 3 primary states in line with the spritesheet format
	switch (enemy_state) {

	case ENEMY_STATE_ID::Idle:
		enemy_current_state = ENEMY_STATE_ID::Idle;
		// TODO: Change animation setting to call to animation system to change state (instead of changing
		// it here in AI)
		animation.frame = 0;
		animation.state = (int)enemy_state;
		break;

	case ENEMY_STATE_ID::ACTIVE:
		enemy_current_state = ENEMY_STATE_ID::ACTIVE;
		animation.frame = 0;
		animation.state = (int)enemy_state;
		break;

	case ENEMY_STATE_ID::FLINCHED:
		enemy_current_state = ENEMY_STATE_ID::FLINCHED;
		animation.frame = 0;
		animation.state = (int)enemy_state;
		break;

	default:
		throw std::runtime_error("Invalid Enemy State");
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
	// TODO: M2 add animation/sound.
	printf("Enemy becomes alert!\n");
}

void AISystem::attack_player(const Entity& entity)
{
	// TODO: M2 add animation/sound and injure player.
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
	MapPosition& entity_map_pos = registry.map_positions.get(entity);
	if (entity_map_pos.position != map_pos && map_generator->walkable(map_pos)) {
		entity_map_pos.position = map_pos;
		return true;
	}
	return false;
}
