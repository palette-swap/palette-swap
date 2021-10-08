// internal
#include "turn_system.hpp"

Entity TurnSystem::getActiveUnit()
{
	if (!teamQueue.empty()) {
		return teamQueue.front();
	}
	return {};
}

bool TurnSystem::teamInQueue(Entity team)
{
	for (Entity e : teamQueue) {
		if (e == team) {
			return true;
		}
	}
	return false;
}

bool TurnSystem::addTeamToQueue(Entity team)
{
	if (!teamInQueue(team)) {
		teamQueue.push_back(team);
		return true;
	}
	return false;
}

bool TurnSystem::removeTeamFromQueue(Entity team)
{
	for (Entity e : teamQueue) {
		if (e == team) {
			teamQueue.pop_front();
		} else {
			teamQueue.push_back(teamQueue.front());
			teamQueue.pop_front();
		}
	}
	return true;
}

bool TurnSystem::skipTeamAction(Entity team)
{
	executeTeamAction(team);
	return completeTeamAction(team);
}

bool TurnSystem::executeTeamAction(Entity team)
{
	if (getActiveUnit() == team && queueState == QUEUE_STATE::IDLE) {
		queueState = QUEUE_STATE::EXECUTING;
		printf("Executing turn: Team %d\n", (int)team);

		return true;
	}
	return false;
}

bool TurnSystem::completeTeamAction(Entity team)
{
	assert(getActiveUnit() == team);
	if ((queueState == QUEUE_STATE::EXECUTING || queueState == QUEUE_STATE::IDLE) && getActiveUnit() == team) {
		printf("Completing turn of: Team %d\n", (int)team);
		queueState = QUEUE_STATE::FINISHED;
		// Perform post-execution actions
		cycleQueue();
		printf("Current turn: Team %d\n", getActiveUnit());
		return true;
	}
	return false;
}

bool TurnSystem::cycleQueue()
{
	teamQueue.push_back(teamQueue.front());
	teamQueue.pop_front();
	queueState = QUEUE_STATE::IDLE;
	return true;
}