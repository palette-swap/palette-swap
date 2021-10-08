#pragma once
#include <deque>
#include <utility>

#include "common.hpp"
#include "tiny_ecs_registry.hpp"

class TurnSystem {
public:
	Entity get_active_unit();
	// std::deque<Entity>::iterator getTeamPositionInQueue(Entity team);
	bool team_in_queue(Entity team);
	bool execute_team_action(Entity team);
	bool complete_team_action(Entity team);
	bool add_team_to_queue(Entity team);
	bool remove_team_from_queue(Entity team);

	bool skip_team_action(Entity team);

	enum class QueueState { Idle, Executing, Finished };

private:
	QueueState queue_state = QueueState::Idle;

	std::deque<Entity> team_queue;
	bool cycle_queue();
};
/*
struct turn_flag {
	Entity team;
	turn_flag(Entity team) { this.team = team; }
	~turn_flag() {
		TurnSystem::complete_team_action(team);
	}
};*/