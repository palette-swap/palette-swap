// internal
#include "ai_system.hpp"
#include "components.hpp"

// Depends on Turn System from Nathan.

bool AISystem::is_player_turn()
{
	// TODO
	return true;
}

void AISystem::switch_to_player_turn()
{
	// TODO
}

// Depends on Map System from Yan.

bool AISystem::is_player_spotted(uint /*radius*/)
{
	// TODO
	return true;
}

bool AISystem::is_player_reachable()
{
	// TODO
	return true;
}

bool AISystem::is_afraid()
{
	// TODO
	return true;
}

void AISystem::animate_alert()
{
	// TODO
}

void AISystem::approach_player()
{
	// TODO
}

void AISystem::attack_player()
{
	// TODO
}

void AISystem::flee_player()
{
	// TODO
}

// AI logic

void AISystem::step(float /*elapsed_ms*/)
{
	if (is_player_turn()) {
		return;
	}

	for (EnemyState& enemy_state : registry.enemy_states.components) {

		switch (enemy_state.current_state) {
		case ENEMY_STATE_ID::IDLE:

			if (is_player_spotted(5)) {
				animate_alert();
				enemy_state.current_state = ENEMY_STATE_ID::ACTIVE;
			}

			break;

		case ENEMY_STATE_ID::ACTIVE:

			if (is_player_spotted(7)) {
				if (is_player_reachable()) {
					attack_player();
				} else {
					approach_player();
				}

				if (is_afraid()) {
					enemy_state.current_state = ENEMY_STATE_ID::FLINCHED;
				}
			} else {
				enemy_state.current_state = ENEMY_STATE_ID::IDLE;
			}

			break;

		case ENEMY_STATE_ID::FLINCHED:

			if (is_player_spotted(9)) {
				flee_player();
			} else {
				enemy_state.current_state = ENEMY_STATE_ID::IDLE;
			}

			break;

		default:
			printf("Should never happen.\n");
			throw std::runtime_error("Invalid Enemy State");
		}
	}

	switch_to_player_turn();
}
