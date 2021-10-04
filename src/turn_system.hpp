#pragma once
#include <deque>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"



class TurnSystem
{
public:

	Entity getActiveUnit();
	std::deque<Entity> getUnitPositionInQueue(Entity unit);
	bool executeUnitAction(Entity unit);
	bool completeUnitAction(Entity unit);
	bool addUnitToQueue(Entity unit);
	bool removeUnitFromQueue(Entity unit);
	

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
	
	std::deque<Entity> unitQueue;

};