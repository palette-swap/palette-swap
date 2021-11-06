#pragma once
#include <deque>
#include <utility>

#include "common.hpp"

#include "animation_system.hpp"

class TurnSystem {
public:
	void step();

	// Returns the entity who can start their turn / is in their turn
	Entity get_active_team();

	bool ready_to_act(Entity team);
	
	// Returns true if the entity team has a turn
	bool team_in_queue(Entity team);

	// Begins the turn of the given entity if they are the active team and we're currently idle
	bool execute_team_action(Entity team);

	// Ends the turn of the given entity, makes the next team in the queue active
	bool complete_team_action(Entity team);
	
	// Adds a turn associated with the provided entity
	bool add_team_to_queue(Entity team);

	// Removes the turn associated with the provided entity
	bool remove_team_from_queue(Entity team);

	// Instantly start and end the turn of the given team if it's active
	bool skip_team_action(Entity team);

	// Returns currently active color
	ColorState get_active_color();

	// Returns inactive color (Returns Red if active color is neither red nor blue)
	ColorState get_inactive_color();
	
	// Sets active color
	bool set_active_color(ColorState color);

	enum class QueueState { Idle, Executing, Finished };

private:
	std::shared_ptr<AnimationSystem> animations;

	QueueState queue_state = QueueState::Idle;

	std::deque<Entity> team_queue;
	// Rotates the queue to the next unit and places current unit at the back of the queue.
	bool try_cycle_queue();

	ColorState active_color = ColorState::Red; 
};
/*
struct turn_flag {
	Entity team;
	turn_flag(Entity team) { this.team = team; }
	~turn_flag() {
		TurnSystem::complete_team_action(team);
	}
};*/