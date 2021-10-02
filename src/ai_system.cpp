// internal
#include "ai_system.hpp"
#include "components.hpp"

// Depends on Turn System

bool isPlayerTurn() {
	// TODO
	return true;
}

void switchToPlayerTurn() {
	// TODO
}

// Depends on Map System.

bool isPlayerSpotted(uint radius)
{
	// TODO
	return true;
}

bool isPlayerReachable()
{
	// TODO
	return true;
}

bool isAfraid()
{
	// TODO
	return true;
}

void animateAlert()
{
	// TODO
}

void approachPlayer()
{
	// TODO
}

void attackPlayer()
{
	// TODO
}

void fleePlayer()
{
	// TODO
}


void AISystem::step(float elapsed_ms)
{
	if (isPlayerTurn())
	{
		return;
	}

	for (EnemyState& enemyState : registry.enemyStates.components) {

		switch (enemyState.current_state)
		{
		case ENEMY_STATE_ID::IDLE:

			if (isPlayerSpotted(5))
			{
				animateAlert();
				enemyState.current_state = ENEMY_STATE_ID::ACTIVE;
			}

			break;

		case ENEMY_STATE_ID::ACTIVE:

			if (isPlayerSpotted(7))
			{
				if (isPlayerReachable())
				{
					attackPlayer();
				}
				else
				{
					approachPlayer();
				}

				if (isAfraid())
				{
					enemyState.current_state = ENEMY_STATE_ID::FLINCHED;
				}
			}
			else
			{
				enemyState.current_state = ENEMY_STATE_ID::IDLE;
			}

			break;

		case ENEMY_STATE_ID::FLINCHED:

			if (isPlayerSpotted(9))
			{
				fleePlayer();
			}
			else
			{
				enemyState.current_state = ENEMY_STATE_ID::IDLE;
			}

			break;

		default:
			printf("Should never happen.\n"); exit(1);
		}
	}

	switchToPlayerTurn();
}
