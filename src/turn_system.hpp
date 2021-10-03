#pragma once

#include <deque>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

using namespace std;

class TurnSystem
{
public:

	Entity getActiveUnit();
	deque<Entity> getUnitPositionInQueue(Entity unit);
	bool executeUnitAction();
	bool completeUnitAction();
	bool addUnitToQueue(Entity unit);
	bool removeUnitFromQueue(Entity unit);
	

	enum class QUEUE_STATE
	{
		IDLE, 
		EXECUTING,
		FINISHED
	};

	QUEUE_STATE _queueState = QUEUE_STATE::IDLE;

	TurnSystem() 
	{
	}

private:
	
	deque<Entity> _unitQueue;

};