#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"


// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);
// the enemy
Entity createEnemy(RenderSystem* renderer, vec2 position);
// Arrows used by player
Entity createArrow(RenderSystem* renderer, vec2 position);
// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);



