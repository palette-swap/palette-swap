#pragma once

#include "common.hpp"
#include "render_system.hpp"
#include "tiny_ecs.hpp"

// the player
Entity create_player(RenderSystem* renderer, vec2 pos);
// the enemy
Entity create_enemy(RenderSystem* renderer, vec2 position);
// a red line for debugging purposes
Entity create_line(vec2 position, vec2 size);
