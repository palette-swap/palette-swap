// internal
#include "turn_system.hpp"

void TurnSystem::step() { 
	if (queue_state == QueueState::Finished) {
		try_cycle_queue();
	}
}

Entity TurnSystem::get_active_team()
{
	if (!team_queue.empty()) {
		return team_queue.front();
	}
	return {};
}

bool TurnSystem::ready_to_act(Entity team) { return get_active_team() == team && queue_state == QueueState::Idle; }

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
	size_t size = team_queue.size();
	for (size_t i = 0; i < size; i++) {
		Entity e = team_queue.front();
		if (e == team) {
			team_queue.pop_front();
			return true;
		}

		team_queue.push_back(e);
		team_queue.pop_front();
	}
	return false;
}

bool TurnSystem::skip_team_action(Entity team)
{
	execute_team_action(team);
	return complete_team_action(team);
}

ColorState TurnSystem::get_active_color() { return activeColor; }

ColorState TurnSystem::get_inactive_color() { 
	return (activeColor == ColorState::Red) ? ColorState::Blue : ColorState::Red;
}

bool TurnSystem::set_active_color(ColorState color) { 
	activeColor = color;
	printf("Current color: %d\n", activeColor);
	return true; 
}

bool TurnSystem::execute_team_action(Entity team)
{
	if (get_active_team() == team && queue_state == QueueState::Idle) {
		queue_state = QueueState::Executing;
		return true;
	}
	return false;
}

bool TurnSystem::complete_team_action(Entity team)
{
	assert(get_active_team() == team);
	if ((queue_state == QueueState::Executing || queue_state == QueueState::Idle) && get_active_team() == team) {
		queue_state = QueueState::Finished;
		// Perform post-execution actions
		return try_cycle_queue();
	}
	return false;
}

bool TurnSystem::try_cycle_queue()
{
	if (animations->animation_events_completed() && queue_state == QueueState::Finished) {
		team_queue.push_back(team_queue.front());
		team_queue.pop_front();
		queue_state = QueueState::Idle;
		return true;
	}
	return false;
}