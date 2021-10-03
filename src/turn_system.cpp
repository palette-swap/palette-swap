//internal
#include <deque>

#include "turn_system.hpp"

Entity TurnSystem::getActiveUnit()
{
	return _unitQueue.front();
}

deque<Entity> TurnSystem::getUnitPositionInQueue(Entity unit)
{
	return std::find(_unitQueue.begin(), _unitQueue.end(), unit);
}

bool TurnSystem::addUnitToQueue(Entity unit)
{
	if (getUnitPositionInQueue(unit) != _unitQueue.end()) {
		_unitQueue.push_back(unit);
		return true;
	}
	else return false;
}

bool TurnSystem::removeUnitFromQueue(Entity unit)
{
	_unitQueue.erase(getUnitPositionInQueue(unit));
	return true;
}

bool TurnSystem::executeUnitAction()
{
	if (_queueState == QUEUE_STATE::IDLE) {
		_queueState = QUEUE_STATE::EXECUTING;
		return true;
	}
	return false;
	
}

bool TurnSystem::completeUnitAction()
{
	if (_queueState == QUEUE_STATE::EXECUTING || _queueState == QUEUE_STATE::IDLE) {
		_queueState == QUEUE_STATE::FINISHED;
		// Perform post-execution actions
		_unitQueue.push_back(_unitQueue.front());
		_unitQueue.pop_front();
		_queueState == QUEUE_STATE::IDLE;
		return true;
	}
	return false;
}
