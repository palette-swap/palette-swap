#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"

// These are ahrd coded to the dimensions of the entity texture
const float FISH_BB_WIDTH = 0.4f * 296.f;
const float FISH_BB_HEIGHT = 0.4f * 165.f;
const float TURTLE_BB_WIDTH = 0.4f * 300.f;
const float TURTLE_BB_HEIGHT = 0.4f * 202.f;

const float ROOM_SPRITE_WIDTH = 32.f;
const float ROOM_SPRITE_HEIGHT = 32.f;

// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);
// the enemy
Entity createTurtle(RenderSystem* renderer, vec2 position);
// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);
// a pebble
Entity createPebble(vec2 pos, vec2 size);

// Creates a room on the current world map
Entity createRoom(RenderSystem* renderer, vec2 position, RoomType roomType);

// Generates the levels, should be called at the beginning of the game,
// might want to pass in a seed in the future
Entity generateMap(RenderSystem* renderer);

