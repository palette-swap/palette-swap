// internal
#include "turn_system.hpp"

Entity TurnSystem::get_active_team()
{
	if (!team_queue.empty()) {
		return team_queue.front();
	}
	return {};
}

bool TurnSystem::team_in_queue(Entity team)
{
	for (Entity e : team_queue) {
		if (e == team) {
			return true;
		}
	}
	return false;
}

bool TurnSystem::add_team_to_queue(Entity team)
{
	if (!team_in_queue(team)) {
		team_queue.push_back(team);
		return true;
	}
	return false;
}

bool TurnSystem::remove_team_from_queue(Entity team)
{
	for (Entity e : team_queue) {
		if (e == team) {
			team_queue.pop_front();
		} else {
			team_queue.push_back(team_queue.front());
			team_queue.pop_front();
		}
	}
	return true;
}

bool TurnSystem::skip_team_action(Entity team)
{
	execute_team_action(team);
	return complete_team_action(team);
}

bool TurnSystem::execute_team_action(Entity team)
{
	if (get_active_team() == team && queue_state == QueueState::Idle) {
		queue_state = QueueState::Executing;
		printf("Executing turn: Team %d\n", (int)team);

		return true;
	}
	return false;
}

bool TurnSystem::complete_team_action(Entity team)
{
	assert(get_active_team() == team);
	if ((queue_state == QueueState::Executing || queue_state == QueueState::Idle) && get_active_team() == team) {
		printf("Completing turn of: Team %d\n", (int)team);
		queue_state = QueueState::Finished;
		// Perform post-execution actions
		cycle_queue();
		printf("Current turn: Team %d\n", get_active_team());
		return true;
	}
	return false;
}

bool TurnSystem::cycle_queue()
{
	team_queue.push_back(team_queue.front());
	team_queue.pop_front();
	queue_state = QueueState::Idle;
	return true;
}