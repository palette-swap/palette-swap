#pragma once

#include "common.hpp"
#include "render_system.hpp"
#include "tiny_ecs.hpp"

// These are ahrd coded to the dimensions of the entity texture
const float room_sprite_width = 32.f;
const float room_sprite_height = 32.f;

// the player
Entity create_player(RenderSystem* renderer, uvec2 pos);
// the enemy
Entity create_enemy(RenderSystem* renderer, vec2 position);
// a red line for debugging purposes
Entity create_line(vec2 position, vec2 scale);

// Creates a room on the current world map
Entity create_room(RenderSystem* renderer, vec2 position, RoomType roomType);
