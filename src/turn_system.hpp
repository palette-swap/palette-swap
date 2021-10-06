#pragma once
#include <deque>
#include <utility>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

class TurnSystem
{
public:

	Entity getActiveUnit();
	std::deque<Entity> getTeamPositionInQueue(Entity team);
	bool teamInQueue(Entity team);
	std::unique_ptr<turn_flag> executeTeamAction(Entity team);
	bool completeTeamAction(Entity team);
	bool addTeamToQueue(Entity team);
	bool removeTeamFromQueue(Entity team);
	

	enum class QUEUE_STATE
	{
		IDLE, 
		EXECUTING,
		FINISHED
	};

	QUEUE_STATE queueState = QUEUE_STATE::IDLE;

	TurnSystem() 
	{
	}

private:
	
	std::deque<Entity> teamQueue;

};

struct turn_flag {
	Entity team;
	turn_flag(Entity team) { this.team = team; }
	~turn_flag() {
		TurnSystem::completeTeamAction(team);
	}
};