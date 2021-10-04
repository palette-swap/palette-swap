//internal
#include <deque>

#include "turn_system.hpp"

Entity TurnSystem::getActiveUnit()
{
	return unitQueue.front();
}

deque<Entity> TurnSystem::getUnitPositionInQueue(Entity unit)
{
	return std::find(unitQueue.begin(), unitQueue.end(), unit);
}

bool TurnSystem::addUnitToQueue(Entity unit)
{
	if (getUnitPositionInQueue(unit) == unitQueue.end()) {
		unitQueue.push_back(unit);
		return true;
	}
	else return false;
}

bool TurnSystem::removeUnitFromQueue(Entity unit)
{
	unitQueue.erase(getUnitPositionInQueue(unit));
	return true;
}

bool TurnSystem::executeUnitAction(Entity unit)
{
	if (getActiveUnit() == unit && queueState == QUEUE_STATE::IDLE) {
		queueState = QUEUE_STATE::EXECUTING;
		return true;
	}
	return false;
	
}

bool TurnSystem::completeUnitAction(Entity unit)
{
	if ((queueState == QUEUE_STATE::EXECUTING || queueState == QUEUE_STATE::IDLE)
				&& getActiveUnit() == unit) {
		queueState == QUEUE_STATE::FINISHED;
		// Perform post-execution actions
		unitQueue.push_back(unitQueue.front());
		unitQueue.pop_front();
		queueState == QUEUE_STATE::IDLE;
		return true;
	}
	return false;
}
