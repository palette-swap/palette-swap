//internal
#include "turn_system.hpp"

Entity TurnSystem::getActiveUnit()
{
	return teamQueue.front();
}

std::deque<Entity> TurnSystem::getTeamPositionInQueue(Entity unit)
{
	return std::find(teamQueue.begin(), teamQueue.end(), unit);
}

bool TurnSystem::teamInQueue(Entity team) 
{
	return getTeamPositionInQueue(team) == teamQueue.end();
}

bool TurnSystem::addTeamToQueue(Entity team)
{
	if (teamInQueue(team)) {
		teamQueue.push_back(team);
		return true;
	}
	else return false;
}

bool TurnSystem::removeTeamFromQueue(Entity team)
{
	teamQueue.erase(getTeamPositionInQueue(team));
	return true;
}

std::unique_ptr<turn_flag> TurnSystem::executeTeamAction(Entity team)
{
	
	if (getActiveUnit() == team && queueState == QUEUE_STATE::IDLE) {
		queueState = QUEUE_STATE::EXECUTING;
		return std::make_unique<turn_flag>(team);
	}
	return NULL;
}

bool TurnSystem::completeTeamAction(Entity team)
{
	if ((queueState == QUEUE_STATE::EXECUTING || queueState == QUEUE_STATE::IDLE)
				&& getActiveUnit() == team) {
		queueState == QUEUE_STATE::FINISHED;
		// Perform post-execution actions
		cycleQueue();
		return true;
	}
	return false;
}

bool cycleQueue() {
	teamQueue.push_back(teamQueue.front());
	teamQueue.pop_front();
	queueState == QUEUE_STATE::IDLE;
}
