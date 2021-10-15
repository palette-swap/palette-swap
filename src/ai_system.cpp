// internal
#include "ai_system.hpp"
#include "components.hpp"

// AI logic
AISystem::AISystem(std::shared_ptr<MapGeneratorSystem> map_generator, std::shared_ptr<TurnSystem> turns)
	: map_generator(std::move(map_generator))
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

	for (const Entity& enemy_entity : registry.enemy_states.entities) {
		
		EnemyState enemy_state = registry.enemy_states.get(enemy_entity);
		uint8_t active_world_color = (uint8_t)turns->get_active_color();

		if ((active_world_color & enemy_state.team) > 0) {
			switch (enemy_state.current_state) {

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
	}

	turns->complete_team_action(enemy_team);
}

void AISystem::switch_enemy_state(const Entity& enemy_entity, ENEMY_STATE_ID enemy_state)
{
	ENEMY_STATE_ID& enemy_current_state = registry.enemy_states.get(enemy_entity).current_state;
	TEXTURE_ASSET_ID& enemy_current_textrue = registry.render_requests.get(enemy_entity).used_texture;
	vec3& enemy_current_color = registry.colors.get(enemy_entity);

	switch (enemy_state) {

	case ENEMY_STATE_ID::Idle:
		enemy_current_state = ENEMY_STATE_ID::Idle;
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME;
		enemy_current_color = { 1, 1, 1 };
		break;

	case ENEMY_STATE_ID::ACTIVE:
		enemy_current_state = ENEMY_STATE_ID::ACTIVE;
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_ALERT;
		enemy_current_color = { 3, 1, 1 };
		break;

	case ENEMY_STATE_ID::FLINCHED:
		enemy_current_state = ENEMY_STATE_ID::FLINCHED;
		enemy_current_textrue = TEXTURE_ASSET_ID::SLIME_FLINCHED;
		enemy_current_color = { 1, 1, 1 };
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

bool AISystem::is_afraid(const Entity& /*entity*/)
{
	// TODO: M2 whether enemies is afraid depends on their real HP.
	uint hp = uint(uniform_dist(rng) * 100.f);
	printf("Enemy HP: %d\n", hp);
	if (hp < 5) {
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

void AISystem::attack_player(const Entity& /*entity*/)
{
	// TODO: M2 add animation/sound and injure player.
	printf("Enemy attacks player!\n");
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
