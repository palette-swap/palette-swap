#pragma once

#include <vector>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"


class AISystem
{
public:
	void step(float /*elapsed_ms*/);

	// Depends on Turn System from Nathan.

	bool is_player_turn();

	void switch_to_player_turn();

	// Depends on Map System from Yan.
	bool is_player_spotted(uint /*radius*/);

	bool is_player_reachable();

	bool is_afraid();

	void animate_alert();

	void approach_player();

	void attack_player();

	void flee_player();
};