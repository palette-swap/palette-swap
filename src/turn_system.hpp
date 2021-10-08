#pragma once
#include <deque>
#include <utility>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

class TurnSystem
{
public:
	Entity getActiveUnit();
	//std::deque<Entity>::iterator getTeamPositionInQueue(Entity team);
	bool teamInQueue(Entity team);
	int executeTeamAction(Entity team);
	bool completeTeamAction(Entity team);
	bool addTeamToQueue(Entity team);
	bool removeTeamFromQueue(Entity team);
	
	bool skipTeamAction(Entity team);

	enum class QUEUE_STATE
	{
		IDLE, 
		EXECUTING,
		FINISHED
	};

	QUEUE_STATE queueState = QUEUE_STATE::IDLE;

	static TurnSystem* getInstance();

	TurnSystem();
private:

	static TurnSystem* instance;
	std::deque<Entity> teamQueue;
	bool cycleQueue();
};
/*
struct turn_flag {
	Entity team;
	turn_flag(Entity team) { this.team = team; }
	~turn_flag() {
		TurnSystem::completeTeamAction(team);
	}
};*/